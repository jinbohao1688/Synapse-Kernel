# Synapse Kernel v0.1.0 Release Notes

## 🎉 发布亮点

### 1. **AI原生内核架构**
- 世界上第一个为人工智能原生设计的操作系统内核
- 内核层面的AI能力集成
- 简化的AI友好API设计

### 2. **核心功能实现**
- ✅ **引导程序**: 支持Multiboot2协议
- ✅ **内存管理**: 伙伴分配器 + 虚拟内存
- ✅ **进程管理**: 抢占式调度器 + 进程间通信
- ✅ **文件系统**: VFS + RamFS + TmpFS
- ✅ **ELF加载器**: 支持运行用户程序
- ✅ **系统调用**: 11个核心系统调用
- ✅ **设备驱动**: VGA、键盘、串口支持

### 3. **用户开发套件**
- 完整的用户程序开发套件
- 简单的系统调用接口
- 示范应用程序

## 📋 系统调用列表

| 编号 | 名称 | 功能 |
|------|------|------|
| 0 | SYS_exit | 退出当前进程 |
| 1 | SYS_fork | 创建新进程 |
| 2 | SYS_wait | 等待子进程退出 |
| 3 | SYS_write | 写入文件描述符 |
| 4 | SYS_read | 读取文件描述符 |
| 5 | SYS_open | 打开文件或设备 |
| 6 | SYS_close | 关闭文件描述符 |
| 7 | SYS_mmap | 内存映射 |
| 8 | SYS_munmap | 取消内存映射 |
| 9 | SYS_sbrk | 调整进程堆大小 |
| 10 | SYS_sleep | 进程睡眠 |
| 11 | SYS_execve | 执行程序 |

## 🚀 快速开始

### 安装依赖

**Windows (MSYS2)**: 
```bash
pacman -Syu
pacman -S mingw-w64-ucrt-x86_64-nasm mingw-w64-ucrt-x86_64-gcc make qemu-system-i386
```

**Linux (Ubuntu/Debian)**: 
```bash
sudo apt-get update
sudo apt-get install build-essential nasm qemu-system-x86
```

### 构建和运行

```bash
git clone https://github.com/synapse-os/synapse-kernel.git
cd synapse-kernel
export CROSS_COMPILE=i686-elf-
make
make run
```

## 📁 项目结构

```
synapse-kernel/
├── kernel/               # 内核源代码
├── user-lib/            # 用户程序开发套件
├── apps/                # 示范应用
├── tools/               # 构建工具
├── docs/                # 文档
└── README.md            # 项目门户
```

## 📚 文档

- [构建指南](docs/BUILD.md)
- [系统调用手册](docs/SYSCALLS.md)
- [程序移植指南](docs/PORTING.md)

## 🛠️ 开发工具

### 构建命令

```bash
make             # 构建内核
make apps         # 构建用户应用
make clean        # 清理构建产物
make run          # 在QEMU中运行内核
make debug        # 带调试输出运行
make gdb          # 带GDB支持运行
```

## 🤝 贡献指南

我们欢迎社区贡献！请按照以下步骤参与：

1. Fork本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交修改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 提交Pull Request

## 📄 许可证

Synapse Kernel采用MIT许可证，详情请查看[LICENSE](LICENSE)文件。

## 📞 联系方式

- **GitHub**: [synapse-os/synapse-kernel](https://github.com/synapse-os/synapse-kernel)
- **Discord**: [Synapse OS Community](https://discord.gg/synapse-os)
- **Email**: contact@synapse-os.org

## 🚀 未来规划

### Version 0.2.0 (Next Release)
- [ ] Add 8 more system calls
- [ ] Implement basic network stack
- [ ] Add FAT file system support
- [ ] Enhance process scheduling with priorities
- [ ] Add signal handling

---

**Synapse Kernel**: 重新定义操作系统与AI的交互方式
