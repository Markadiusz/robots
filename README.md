# robots
A project for a Computer networks course at MiMUW.

A detailed description of the project in Polish can be found [here](https://github.com/agluszak/mimuw-sik-2022-public), while [here](https://github-com.translate.goog/agluszak/mimuw-sik-2022-public?_x_tr_sl=pl&_x_tr_tl=en&_x_tr_hl=en-US&_x_tr_pto=wapp) you can find an automatically translated version.

## Setup

### Client and server

The installation of the [GCC](https://gcc.gnu.org/) compiler version at least 10.1 and the [Boost](https://www.boost.org/) library is required.

### GUI

The installation of the [Rust](https://rustup.rs/) compiler and the [Bevy](https://github.com/bevyengine/bevy/blob/main/docs/linux_dependencies.md) library is required.

## Example usage

The program options can be displayed with the --help option.

### Server

    make
    ./robots-server -b 20 -c 1 -d 100 -e 2 -k 5 -l 1200 -p 4321 -n "A normal server name" -x 6 -y 6

### Client

    make
    ./robots-client -d localhost:9876 -p 12345 -n "An intriguing player name" -s localhost:4321

### GUI

    cargo run --release --bin gui -- -c localhost:12345 -p 9876

## Player actions

W, Up Arrow - moves the player up

S, Down Arrow - moves the player down

A, Left Arrow - moves the player to the left

D, Right Arrow - moves the player to the right

Space, J, Z - plants a bomb

K, X - blocks a field
