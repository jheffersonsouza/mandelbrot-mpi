import json
import os
import shlex
import socket
import sys

from .hosts import HOSTS_FILE, ROOT, read_hosts, run, ssh

REMOTE = '''echo mpich=$(mpichversion 2>/dev/null | sed -n 's/^MPICH Version: *//p')
echo commit=$(git -C {root} rev-parse --short HEAD 2>/dev/null)
echo threads=$(nproc)
test -x {root}/bin/mandelbrot_mpi && echo built=yes'''

COLUMNS = ['Host', 'Address', 'MPICH', 'Commit', 'Threads', 'Processes', 'Status']


def ipv4(node):
    return next((ip for ip in node.get('TailscaleIPs') or [] if ':' not in ip), None)


def mpich_version():
    ok, out = run(['mpichversion'])
    return next((line.split(':', 1)[1].strip() for line in out.splitlines()
                 if ok and line.startswith('MPICH Version')), None)


def port_open(port):
    try:
        socket.create_connection(('127.0.0.1', port), timeout=2).close()
        return True
    except OSError:
        return False


def check_local(status, hosts, problems, notes):
    running = status is not None and status.get('BackendState') == 'Running'
    my_ip = ipv4(status['Self']) if running else None
    name = status['Self']['DNSName'].split('.')[0] if running else socket.gethostname()
    found = len(problems)

    try:
        socket.create_connection(('github.com', 443), timeout=3).close()
    except OSError as error:
        problems.append(f'{name}: no internet ({error})')
    if not running:
        problems.append(f'{name}: Tailscale is not connected')
    elif not os.path.exists('/sys/class/net/tailscale0'):
        problems.append(f'{name}: no tailscale0 interface')
    mpich = mpich_version()
    if not mpich:
        problems.append(f'{name}: MPICH not found')
    if not os.access(os.path.join(ROOT, 'bin', 'mandelbrot_mpi'), os.X_OK):
        problems.append(f'{name}: bin/mandelbrot_mpi not built')
    ok, commit = run(['git', '-C', ROOT, 'rev-parse', '--short', 'HEAD'])
    commit = commit.strip() if ok else None
    if not commit:
        problems.append(f'{name}: {ROOT} is not a git clone')

    ips = [ip for ip, _ in hosts]
    if my_ip not in ips:
        role = 'unlisted'
        problems.append(f'{name}: {my_ip} is not in hosts.txt')
    elif my_ip == ips[0]:
        role = 'master'
        if 'DISPLAY' not in os.environ:
            notes.append(f'{name}: no DISPLAY, the image window will not open')
    else:
        role = 'worker'
        ok, prefs = run(['tailscale', 'debug', 'prefs'])
        if ok and json.loads(prefs).get('RunSSH'):
            problems.append(f'{name}: Tailscale SSH answers port 22')
        if not port_open(22):
            problems.append(f'{name}: OpenSSH is not listening on port 22')

    row = [name, my_ip or '-', mpich or '-', commit or '-', str(os.cpu_count()),
           str(dict(hosts).get(my_ip, '-')), role if len(problems) == found else 'fail']
    return row, role, mpich, commit


def check_worker(ip, procs, status, mpich, commit, problems, notes):
    peers = (status or {}).get('Peer') or {}
    peer = next((p for p in peers.values() if ip in (p.get('TailscaleIPs') or [])), None)
    name = peer['DNSName'].split('.')[0] if peer else ip
    row = [name, ip, '-', '-', '-', str(procs), 'fail']
    if not (peer and peer.get('Online')):
        problems.append(f'{name}: offline in Tailscale')
        row[-1] = 'offline'
        return row
    ok, out = ssh(ip, REMOTE.format(root=shlex.quote(ROOT)))
    if not ok:
        problems.append(f'{name}: ssh failed ({out})')
        return row

    info = dict(line.split('=', 1) for line in out.splitlines() if '=' in line)
    row[2:5] = [info.get('mpich') or '-', info.get('commit') or '-', info.get('threads') or '-']
    found = len(problems)
    if info.get('mpich') != mpich:
        problems.append(f'{name}: MPICH {info.get("mpich") or "not found"}, the master has {mpich}')
    if info.get('built') != 'yes':
        problems.append(f'{name}: {ROOT}/bin/mandelbrot_mpi not built')
    if info.get('commit') != commit:
        problems.append(f'{name}: commit {info.get("commit") or "-"}, the master has {commit}')
    threads = int(info.get('threads') or 0)
    if procs > threads:
        notes.append(f'{name}: {procs} processes in hosts.txt, more than its {threads} threads')
    row[-1] = 'ok' if len(problems) == found else 'fail'
    return row


def print_table(rows):
    widths = [max(len(row[i]) for row in [COLUMNS] + rows) for i in range(len(COLUMNS))]
    for row in [COLUMNS] + rows:
        cells = [cell.rjust(width) if column in ('Threads', 'Processes') else cell.ljust(width)
                 for column, cell, width in zip(COLUMNS, row, widths)]
        print('  '.join(cells).rstrip())


def main():
    try:
        hosts = read_hosts()
    except (OSError, ValueError) as error:
        sys.exit(f'Cannot read {HOSTS_FILE}: {error}')
    ok, out = run(['tailscale', 'status', '--json'])
    status = json.loads(out) if ok else None

    problems, notes = [], []
    row, role, mpich, commit = check_local(status, hosts, problems, notes)
    rows = [row]
    if role == 'master':
        rows += [check_worker(ip, procs, status, mpich, commit, problems, notes)
                 for ip, procs in hosts[1:]]

    print_table(rows)
    if problems or notes:
        print()
        print('\n'.join(problems + notes))
    sys.exit(1 if problems else 0)
