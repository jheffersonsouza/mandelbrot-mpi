import os
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HOSTS_FILE = os.path.join(ROOT, 'hosts.txt')


def read_hosts(path=HOSTS_FILE):
    hosts = []
    with open(path) as lines:
        for line in lines:
            line = line.strip()
            if line:
                ip, _, procs = line.partition(':')
                hosts.append((ip, int(procs or 1)))
    return hosts


def run(command, timeout=10, env=None):
    try:
        result = subprocess.run(command, capture_output=True, text=True, timeout=timeout, env=env)
    except FileNotFoundError:
        return False, f'{command[0]} not found'
    except subprocess.TimeoutExpired:
        return False, f'no answer in {timeout} s'
    if result.returncode != 0:
        return False, (result.stderr.strip().splitlines() or ['failed'])[-1]
    return True, result.stdout


def ssh(ip, command, timeout=15):
    return run(['ssh', '-o', 'BatchMode=yes', '-o', 'ConnectTimeout=5', ip, command], timeout)
