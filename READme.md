# Chatter 💬

Chatter is a **terminal-based peer-to-peer (P2P) chat application** that allows two users on the same network to communicate using sockets — no centralized server required!

---

## ✨ Features

- 🔄 **Peer-to-peer chatting** (no server)
- 🔧 **Command-based interface** for easy messaging
- 🧵 **Multithreaded message handling** to ensure smooth communication
- 📡 **Local broadcast for peer discovery**, making it easy to find others on the same network
- 🔚 **Graceful exit handling** to cleanly shut down the application and terminate threads

---

## 🛠 Requirements

- **GCC** – The GNU Compiler Collection for compiling C programs.
- **Make** – A build automation tool to simplify the compilation process.
- **Pthreads** – POSIX threads library (usually preinstalled on Linux).
- **Linux-based system** – Compatible with Ubuntu, Fedora, and other Linux distros.

---

## 📥 Installation

### 🔁 Option 1: Install via Debian package

1. **Clone the repository**:

   ```bash
   git clone https://github.com/taaha-0548/Chatter
   cd Chatter
   ```

2. **Install the package**:

   ```bash
   sudo dpkg -i chatter-2.1.deb
   ```

3. **Run Chatter**:

   ```bash
   chatter
   ```

---

### 🔧 Option 2: Build from source

1. **Clone the repository**:

   ```bash
   git clone https://github.com/taaha-0548/Chatter
   cd Chatter
   ```

2. **Build using Make**:

   ```bash
   make
   ```

3. **Run the app**:

   ```bash
   ./chatter
   ```

---

## 📁 Project Structure

```
Chatter/
├── include/
│   └── chatter.h
├── src/
│   ├── commands.c
│   ├── main.c
│   ├── network.c
│   ├── ui.c
│   └── utils.c
├── chatter-2.1.deb
├── Makefile
└── README.md
```

---

Enjoy chatting with Chatter! 😄
