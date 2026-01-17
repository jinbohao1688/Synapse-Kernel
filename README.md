# Synapse Kernel - 世界首个AI原生操作系统内核

[![Build Status](https://img.shields.io/badge/build-in_progress-yellow.svg)](https://github.com/synapse-os/synapse-kernel)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Architecture](https://img.shields.io/badge/arch-i386-red.svg)](https://github.com/synapse-os/synapse-kernel)

Synapse Kernel 是世界上第一个为人工智能原生设计的操作系统内核，重新定义了操作系统与AI的交互方式。它采用了创新的AI优先架构，将AI能力从系统底层进行深度融合，而非作为上层应用叠加。

## 🚀 核心创新

### AI原生架构 vs 传统内核

| 特性 | 传统操作系统内核 | Synapse AI原生内核 |
|------|------------------|-------------------|
| **设计理念** | 为人类用户设计，AI作为应用 | 为AI设计，人类作为辅助 |
| **内存管理** | 基于人类使用模式的分页机制 | 针对AI模型优化的内存管理 |
| **进程调度** | 公平性优先的CPU调度 | AI任务优先级感知调度 |
| **系统调用** | 复杂的POSIX接口 | 简化的AI友好API |
| **文件系统** | 分层文件系统 | 基于AI语义的内容寻址 |
| **安全模型** | 用户-组权限模型 | AI能力分级授权 |
| **调试机制** | 传统gdb调试 | AI辅助的自动调试 |
| **扩展性** | 模块动态加载 | 运行时AI能力扩展 |

### 关键技术特性

- **AI-Native Architecture**: 内核层面的AI能力集成
- **ELF加载器**: 支持运行用户程序和AI模型
- **内存管理**: 伙伴分配器 + 虚拟内存管理
- **进程管理**: 抢占式调度器 + 进程间通信
- **文件系统**: VFS + RamFS + TmpFS
- **系统调用**: 11个核心系统调用
- **设备驱动**: 基本的键盘、VGA、串口支持

## 📋 快速开始

按照以下5个步骤在QEMU中运行Synapse Kernel：

### 1. 安装依赖

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

### 2. 下载i686-elf工具链

- **Windows**: [i686-elf-tools](https://github.com/lordmilko/i686-elf-tools/releases)
- **Linux**: 按照[OSDev指南](https://wiki.osdev.org/GCC_Cross-Compiler)编译

### 3. 克隆仓库

```bash
git clone https://github.com/synapse-os/synapse-kernel.git
cd synapse-kernel
```

### 4. 构建内核

```bash
export PATH="/path/to/i686-elf-tools/bin:$PATH"
export CROSS_COMPILE=i686-elf-
make
```

### 5. 运行内核

```bash
make run
```

## 📊 项目状态

### 已完成模块

- ✅ **引导程序**: 支持Multiboot2协议
- ✅ **内存管理**: 
  - 物理内存管理（伙伴分配器）
  - 虚拟内存管理（分页机制）
  - 内核堆管理
- ✅ **进程管理**: 
  - 进程创建、调度和终止
  - 抢占式调度器
  - 进程间通信
- ✅ **文件系统**: 
  - 虚拟文件系统层（VFS）
  - RamFS实现
  - TmpFS实现
  - ELF加载器
- ✅ **设备驱动**: 
  - VGA文本模式
  - PS/2键盘
  - 串口通信
- ✅ **系统调用**: 11个核心系统调用
- ✅ **AI集成**: 基础AI执行框架

## 📁 项目结构

```
synapse-kernel/
├── kernel/               # 内核源代码
│   ├── boot/            # 引导程序
│   ├── mm/              # 内存管理
│   ├── proc/            # 进程管理
│   ├── fs/              # 文件系统
│   ├── drivers/         # 设备驱动
│   ├── loader/          # ELF加载器
│   └── include/         # 内核头文件
├── user-lib/            # 用户程序开发套件
│   ├── include/         # 用户空间头文件
│   ├── crt0.s           # 用户程序启动桩
│   └── linker.ld        # 用户程序链接脚本
├── apps/                # 示范应用
│   ├── hello/           # Hello World
│   ├── ai_demo/         # AI原生演示程序
│   └── build.sh         # 应用编译脚本
├── tools/               # 构建工具
│   ├── toolchain-setup/ # 交叉编译工具链配置
│   └── qemu-run.sh      # 一键启动脚本
├── docs/                # 文档
│   ├── BUILD.md         # 构建指南
│   ├── SYSCALLS.md      # 系统调用手册
│   └── PORTING.md       # 程序移植指南
├── lib/                 # 内核库
├── sbin/                # 系统服务
├── Makefile             # 顶层构建文件
└── README.md            # 项目门户
```

## 📚 文档

- [构建指南](docs/BUILD.md): 详细的构建和运行说明
- [系统调用手册](docs/SYSCALLS.md): 完整的系统调用参考
- [程序移植指南](docs/PORTING.md): 将程序移植到Synapse Kernel的指南

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

### 交叉编译工具链

Synapse Kernel使用i686-elf交叉编译工具链，详细的工具链设置指南请参考[构建指南](docs/BUILD.md)。

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

- **开发者**: 为由崎司献上心脏
- **邮箱**: 15378707620@163.com

## 🚀 未来规划

- **基础功能**: 核心内核功能已完成
- **网络支持**: 添加基础网络栈和更多文件系统
- **AI安全**: 完善AI安全框架
- **AI能力**: 支持运行大型AI模型
- **智能管理**: AI辅助的系统管理
- **稳定版本**: 生产级稳定性

---

**Synapse Kernel**: 重新定义操作系统与AI的交互方式
