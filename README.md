# NX DNS Manager
![Latest Release](https://img.shields.io/github/v/release/JP3402/NX-DNS-Manager)

A simple homebrew application for the Nintendo Switch to manage DNS MITM hosts files used by Atmosphere.

## Features

-   Dynamically detects and lists available hosts files (`default.txt`, `emummc.txt`, `sysmmc.txt`).
-   Provides a simple UI to enable or disable individual entries in the selected hosts file.
-   Saves changes back to the selected hosts file.

## Installation

1.  Download the latest `dns-mitm-manager.nro` from the [releases page](https://github.com/JP3402/NX-DNS-Manager/releases).
2.  Copy `dns-mitm-manager.nro` to the `/switch/` folder on your SD card.
3.  Launch the application from the Homebrew Menu.

## Usage

1.  On startup, the application will scan your `/atmosphere/hosts/` directory for `default.txt`, `emummc.txt`, and `sysmmc.txt`.
2.  A menu will appear showing the hosts files that were found.
3.  Use the **D-Pad Up/Down** to navigate the list of files and press **A** to select one.
4.  In the main editor screen, use the **D-Pad Up/Down** to select a host entry.
5.  Press **A** to toggle the selected entry on or off. An `[X]` indicates the entry is active, while `[ ]` indicates it is commented out and inactive.
6.  Press **Y** to save your changes back to the file. A "Saved!" message will appear.
7.  Press **+** to exit the application at any time.

## Building from Source

This project is built using `devkitPro` and `libnx`. The easiest way to compile the source code is by using the provided Docker container.

### Prerequisites

-   [Docker](https://www.docker.com/get-started) installed and running on your system.

### Compilation Steps

1.  Clone the repository:
    ```sh
    git clone https://github.com/your-username/dns-mitm-manager.git
    cd dns-mitm-manager
    ```

2.  Run the build command using the `devkitpro/devkita64` Docker image:
    ```sh
    sudo docker run --rm -v $(pwd):/project -w /project devkitpro/devkita64 make
    ```

3.  The compiled `dns-mitm-manager.nro` file will be located in the `out/` directory.

## License

This project is licensed under the ISC License. See the [LICENSE](LICENSE) file for details.
