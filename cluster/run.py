import argparse
import os
import shlex
import subprocess
import sys
import time

from rich.console import Console
from rich.live import Live

from .hosts import HOSTS_FILE, ROOT, read_hosts
from .panel import Run, render


def mpiexec_command(hosts_file, options):
    procs = sum(n for _, n in read_hosts(os.path.join(ROOT, hosts_file)))
    return ['mpiexec', '-f', hosts_file, '-iface', 'tailscale0', '-genvnone',
            '-genv', 'UCX_NET_DEVICES', 'tailscale0', '-n', str(procs),
            './bin/mandelbrot_mpi'] + options


def run_with_panel(console, command):
    run = Run()
    process = subprocess.Popen(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               text=True, errors='replace')
    with Live(render(run), console=console, auto_refresh=False) as live:
        last = 0.0
        while run.exit_code is None:
            try:
                for line in process.stdout:
                    run.feed(line.rstrip('\n'))
                    # redesenha no máximo a cada 0,1 s
                    if not line.startswith(('@sent', '@received')) or time.monotonic() - last > 0.1:
                        live.update(render(run), refresh=True)
                        last = time.monotonic()
                run.exit_code = process.wait()
            except KeyboardInterrupt:
                pass   # o Ctrl+C também chega ao mpiexec, que encerra os processos
        live.update(render(run), refresh=True)
    for line in run.output:
        print(line)
    return run.exit_code


def main():
    parser = argparse.ArgumentParser(
        prog='uv run mandelbrot', allow_abbrev=False,
        description='Runs bin/mandelbrot_mpi with mpiexec on the hosts of hosts.txt. Other '
                    'options go to mandelbrot_mpi: --width N, --height N, --iter N, --block N, '
                    '--full, --no-window.')
    parser.add_argument('--hosts', default=HOSTS_FILE, help='host file (default: hosts.txt)')
    parser.add_argument('--no-panel', action='store_true', help='plain output, without the panel')
    args, options = parser.parse_known_args()

    console = Console(highlight=False)
    panel = console.is_terminal and not args.no_panel
    hosts_file = os.path.relpath(os.path.abspath(args.hosts), ROOT)
    command = mpiexec_command(hosts_file, (['--events'] if panel else []) + options)
    # O mpiexec tira do PWD a pasta de trabalho dos workers.
    os.environ['PWD'] = ROOT
    console.print(shlex.join(command), style='dim', markup=False)

    if panel:
        code = run_with_panel(console, command)
    else:
        try:
            code = subprocess.call(command, cwd=ROOT)
        except KeyboardInterrupt:
            code = 130
    sys.exit(code)
