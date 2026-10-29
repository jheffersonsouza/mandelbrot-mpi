import os
import platform
import socket

from rich.console import Console
from rich.table import Table

from .hosts import run


def read(path):
    try:
        with open(path) as file:
            return file.read().strip()
    except OSError:
        return ''


def specs():
    release = dict(line.split('=', 1) for line in read('/etc/os-release').splitlines() if '=' in line)
    wsl = 'microsoft' in read('/proc/version').lower()

    cpu = {}
    ok, out = run(['lscpu'], env={**os.environ, 'LC_ALL': 'C'})
    for line in out.splitlines() if ok else []:
        key, _, value = line.partition(':')
        cpu[key.strip()] = value.strip()
    cores = int(cpu.get('Core(s) per socket') or 0) * int(cpu.get('Socket(s)') or 1)

    ok, out = run(['mpichversion'])
    mpich = next((line.split(':', 1)[1].strip() for line in out.splitlines()
                  if ok and line.startswith('MPICH Version')), '-')
    ok, ip = run(['tailscale', 'ip', '-4'])
    model = f'{read("/sys/class/dmi/id/sys_vendor")} {read("/sys/class/dmi/id/product_name")}'
    memory = int(read('/proc/meminfo').split()[1]) / 1024 ** 2

    rows = [('Host', socket.gethostname()),
            ('System', release.get('PRETTY_NAME', '-').strip('"') + (' (WSL)' if wsl else '')),
            ('Model', model.strip() or '-'),
            ('CPU', cpu.get('Model name', '-')),
            ('Cores', str(cores or '-')),
            ('Threads', str(os.cpu_count())),
            ('Memory', f'{memory:.1f} GB'),
            ('MPICH', mpich),
            ('Python', platform.python_version()),
            ('Tailscale IP', ip.split()[0] if ok and ip.split() else '-')]
    if wsl:
        ok, version = run(['cmd.exe', '/c', 'ver'])
        rows.insert(2, ('Windows', version.strip() if ok else '-'))
    return rows


def main():
    table = Table(show_header=False, box=None, padding=(0, 2))
    for name, value in specs():
        table.add_row(name, value)
    Console(highlight=False).print(table)
