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
- **Pthreads** – A POSIX threads library, usually preinstalled on Linux, for multithreading support.
- **Linux-based system** – The application is designed to run on Linux-based systems (e.g., Ubuntu, Fedora, etc.).

---

## 📥 Installation

1. **Clone the repository**:

   Open the terminal in VSCode and run:

   ```bash
   git clone https://github.com/taaha-0548/chatter.git
   cd chatter
