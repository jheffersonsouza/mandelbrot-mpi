# Mandelbrot com MPI em três máquinas

Trabalho final (P1) da disciplina de Sistemas Distribuídos, ministrada pelo Prof. Dr. Julio Cesar.
UFC, Campus Jardins de Anita, Itapajé.

Autores: Jhefferson Abrahão Alves Barbosa Souza e Pablo Pinto Barbosa.

O fractal de Mandelbrot do código-base do professor é calculado por várias máquinas com MPI
(MPICH). O master divide a figura em blocos de linhas e entrega os blocos sob demanda aos
workers. As máquinas se ligam pelo Tailscale, e o `mpiexec` inicia os processos nas outras
máquinas pelo ssh.

## Estrutura

```
src/          programa em C++, feito a partir do código-base do professor
cluster/      ferramentas em Python, rodadas com o uv
hosts.txt     máquinas e quantos processos cada uma roda
```

## Preparar cada máquina

Ubuntu 26.04 em todas (no Windows, dentro do WSL), com o código na mesma pasta,
`/opt/mandelbrot-mpi`:

```
sudo apt update
sudo apt install mpich libmpich-dev libx11-dev make g++ git curl openssh-server
sudo systemctl enable --now ssh
curl -LsSf https://astral.sh/uv/install.sh | sh
curl -fsSL https://tailscale.com/install.sh | sh
sudo tailscale up --hostname=<nome da máquina>
sudo mkdir -p /opt/mandelbrot-mpi && sudo chown $USER /opt/mandelbrot-mpi
git clone https://github.com/jheffersonsouza/mandelbrot-mpi /opt/mandelbrot-mpi
cd /opt/mandelbrot-mpi && make
```

No master, uma chave ssh sem senha, copiada para cada worker:

```
ssh-keygen -t ed25519 -f ~/.ssh/mandelbrot_mpi_ed25519 -N ''
ssh-copy-id -i ~/.ssh/mandelbrot_mpi_ed25519.pub pablo@100.117.104.66
ssh-copy-id -i ~/.ssh/mandelbrot_mpi_ed25519.pub jheff@100.97.108.65
```

O `~/.ssh/config` do master diz o usuário e a chave de cada worker:

```
Host 100.117.104.66
    User pablo
    IdentityFile ~/.ssh/mandelbrot_mpi_ed25519

Host 100.97.108.65
    User jheff
    IdentityFile ~/.ssh/mandelbrot_mpi_ed25519
```

## Rodar

O `hosts.txt` tem uma máquina por linha, no formato `IP:processos`, com o master primeiro. No
master:

```
uv run preflight
uv run mandelbrot
```

O `uv run preflight` confere as máquinas, e o `uv run mandelbrot` roda o fractal em todas e mostra
a fila de blocos. Opções: `--width N`, `--height N`, `--iter N`, `--block N`, `--full`,
`--no-window`, `--no-panel` e `--hosts arquivo`. O `uv run specs` mostra as specs da máquina, e a
versão sequencial roda com `./bin/mandelbrot_seq`.
