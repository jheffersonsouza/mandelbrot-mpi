import collections
import time

from rich.console import Group
from rich.table import Table
from rich.text import Text

COLORS = ['bright_red', 'bright_green', 'bright_blue', 'bright_yellow', 'bright_magenta', 'bright_cyan']


def plural(count, word):
    return f'{count} {word}' + ('' if count == 1 else 's')


class Run:
    # Estado da fila, montado com as linhas @ que o master escreve com --events.
    def __init__(self):
        self.hosts = []
        self.rank_host = {}
        self.start = {}
        self.blocks = []        # None (esperando), ('queued', worker) ou ('done', worker)
        self.recent = collections.deque(maxlen=8)
        self.output = []
        self.started_at = None
        self.seconds = None
        self.exit_code = None

    def feed(self, line):
        if not line.startswith('@'):
            self.output.append(line)
            return
        try:
            kind, *fields = line[1:].split()
            data = dict(field.split('=', 1) for field in fields)
            if kind == 'rank':
                host = data['host']
                self.rank_host[int(data['rank'])] = host
                if host not in self.hosts:
                    self.hosts.append(host)
            elif kind == 'start':
                self.start = {key: int(value) for key, value in data.items()}
                self.blocks = [None] * self.start['blocks']
                self.started_at = time.monotonic()
            elif kind in ('sent', 'received'):
                block, worker = int(data['block']), int(data['worker'])
                self.blocks[block] = ('queued' if kind == 'sent' else 'done', worker)
                verb = 'sent to    ' if kind == 'sent' else 'computed by'
                self.recent.appendleft(
                    (f'block {block:>4} {verb} {self.rank_host[worker]} (rank {worker})', worker))
            elif kind == 'done':
                self.seconds = float(data['seconds'])
        except (ValueError, KeyError, IndexError):
            self.output.append(line)

    def color(self, worker):
        return COLORS[self.hosts.index(self.rank_host[worker]) % len(COLORS)]

    def lines_in(self, block):
        lines = self.start['lines']
        return min(lines, self.start['height'] - block * lines)

    def host_summary(self, host):
        workers = sum(1 for rank, name in self.rank_host.items() if rank > 0 and name == host)
        queued = done = lines = 0
        for block, state in enumerate(self.blocks):
            if state and self.rank_host[state[1]] == host:
                if state[0] == 'queued':
                    queued += 1
                else:
                    done += 1
                    lines += self.lines_in(block)
        return workers, queued, done, lines


def render(run):
    parts = []
    if run.start:
        start = run.start
        total = len(run.blocks)
        done = sum(1 for state in run.blocks if state and state[0] == 'done')
        queued = sum(1 for state in run.blocks if state and state[0] == 'queued')
        elapsed = run.seconds if run.seconds is not None else time.monotonic() - run.started_at

        parts.append(Text(f'{start["width"]}x{start["height"]} points, {start["iter"]} iterations, '
                          f'{plural(total, "block")} of {plural(start["lines"], "line")}, '
                          f'{start["workers"]} workers'))
        parts.append(Text(f'{done}/{total} computed, {queued} queued, {total - done - queued} '
                          f'waiting, {elapsed:.2f} s'))
        parts.append(Text())

        hosts = Table(box=None, padding=(0, 2), pad_edge=False)
        for name, justify in [('Host', 'left'), ('Workers', 'right'), ('Queued', 'right'),
                              ('Computed', 'right'), ('Image', 'right')]:
            hosts.add_column(name, justify=justify)
        for k, host in enumerate(run.hosts):
            workers, host_queued, host_done, lines = run.host_summary(host)
            name = host + (' (master)' if run.rank_host.get(0) == host else '')
            hosts.add_row(Text(name, style=COLORS[k % len(COLORS)]), str(workers), str(host_queued),
                          str(host_done), f'{100 * lines / start["height"]:.1f}%')
        parts += [hosts, Text()]

        blocks = Text()
        for state in run.blocks:
            if state is None:
                blocks.append('.', style='dim')
            else:
                blocks.append('+' if state[0] == 'queued' else '#', style=run.color(state[1]))
        parts += [Text('Blocks, top to bottom of the image (# computed, + queued, . waiting)'),
                  blocks, Text(), Text('Recent blocks')]
        parts += [Text(text, style=run.color(worker)) for text, worker in run.recent]
    else:
        parts.append(Text('Starting MPI on the hosts...'))

    if run.exit_code is not None:
        status = f'mpiexec finished (exit code {run.exit_code})'
    elif run.seconds is not None:
        status = 'Computation done. If the image window is open, close it to finish.'
    else:
        status = 'Computing... Ctrl+C stops.'
    parts.append(Text())
    if run.output and run.seconds is None:
        parts.append(Text(run.output[-1], style='dim'))
    parts.append(Text(status))
    return Group(*parts)
