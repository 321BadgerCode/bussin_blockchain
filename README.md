<p align="center">
	<img src="./asset/logo.png" alt="Bussin Blockchain Logo" width="200" height="200">
</p>

<h1 align="center">Bussin Blockchain</h1>

<p align="center">
	<strong>Simple blockchain network!</strong>
</p>

## 🚀 Overview

**Bussin Blockchain** is a simple blockchain network that has miners mine blocks and validate transactions. The network is built using the **Proof of Work** consensus algorithm.

> [!WARNING]
> Chat system is not yet implemented as the `captureBlock()` function (it has to do with networking) is not properly scanning for packets being sent in a background thread. It is not appending validated blocks to the global blockchain, so the miners aren't able to keep mining new blocks.

## 🎨 Features

- **Proof of Work** consensus algorithm

## 🛠️ Installation

1. **Clone the repository**
```sh
git clone https://github.com/321BadgerCode/bussin_blockchain.git
cd ./bussin_blockchain/
```

2. **Install the dependencies**
```sh
sudo apt-get install libpcap-dev
```

3. **Build the project**
```sh
g++ ./main.cpp -o ./blockchain.exe -lpcap -lssl -lcrypto -pthread
```

## 📈 Usage

1. **Run the program**
```sh
./blockchain.exe -m <miners>
```

<details>

<summary>💻 Command Line Arguments</summary>

**Command Line Arguments**  
> The difficulty of the blockchain can be adjusted by changing the code `#define DIFFICULTY 4` in the [main](./main.cpp) file.
|	Argument		|	Description	|	Default		|
|	:---:			|	:---:		|	:---:		|
|	`-h & --help`		|	Help menu	|			|
|	`--version`		|	Version number	|			|
|	`-m`			|	Miners		|	`10`		|
|	`-d`			|	Device		|	`enp5s0`	|

</details>

## 📜 License

[LICENSE](./LICENSE)