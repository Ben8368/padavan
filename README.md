# padavan-4.4 — 小米路由器迷你 (MI-MINI) 定制固件

基于 [hanwckf/padavan-4.4](https://github.com/hanwckf/padavan-4.4) 精简并定制的路由器固件项目，专门针对 **小米路由器迷你 (MI-MINI)** 进行 MT7620 平台移植。使用 Linux 4.4.198 内核，参考 rt-n56u 的 MT7620 支持完成驱动适配与硬件 NAT 兼容性修改。

> 本项目是一个 **AI 驱动的嵌入式固件开发实践**，使用 Claude Code + Claude Opus 4.7 全程参与内核移植、驱动调试、构建修复和 CI/CD 自动化，43 个 commit 均在 AI 深度协作下完成。

---

## 项目亮点

| 维度 | 说明 |
|------|------|
| **目标设备** | 小米路由器迷你 (MI-MINI)，MT7620 SoC，128MB RAM |
| **内核版本** | Linux 4.4.198，MIPS32r2 架构 (mipsel) |
| **WiFi** | 双频：2.4G (MT7620 内置) + 5G 802.11ac (MT7612E PCIe) |
| **开发方式** | AI 驱动开发 (Claude Code + Claude Opus 4.7)，43 个 commit 全程 AI 参与 |
| **构建系统** | Makefile + Shell 脚本，GitHub Actions CI/CD 自动化 |
| **许可证** | GPLv2 |

---

## 技术架构

### 系统分层

```
┌─────────────────────────────────────────────────┐
│                   Web UI (ASP)                   │
│            Bootstrap + jQuery + i18n             │
├─────────────────────────────────────────────────┤
│              httpd (HTTP/HTTPS 后端)              │
├─────────────────────────────────────────────────┤
│         rc (init/服务管理器，35个C源文件)          │
│   net_wan · net_wifi · firewall_ex · services    │
├─────────────────────────────────────────────────┤
│   dnsmasq · iptables · wpa_supplicant · pppd     │
│   miniupnpd · iproute2 · libdisk · nvram         │
│   shared · utils                                 │
├─────────────────────────────────────────────────┤
│          Linux 4.4.198 (MIPS32r2)                │
│   MT7620 WiFi · MT7612E PCIe · raeth · HNAT     │
├─────────────────────────────────────────────────┤
│         uClibc-ng 1.0.38 · busybox 1.24.x       │
├─────────────────────────────────────────────────┤
│      MT7620 SoC · 128MB RAM · 16MB Flash         │
└─────────────────────────────────────────────────┘
```

### 内核移植工作 (分阶段完成)

本项目的主要工作是将 MT7620 平台支持从 rt-n56u 移植到 padavan-4.4 的 Linux 4.4 内核：

| 阶段 | 工作内容 | 关键文件 |
|------|---------|---------|
| Phase 1-2 | MT7620 SoC 基础支持，设备树 (DTS) 适配 | `arch/mips/ralink/`, `dts/` |
| Phase 3 | USB PHY DTS 节点，USB 控制器初始化 | `drivers/usb/` |
| Phase 4 | MT7620 内置 2.4G WiFi 驱动适配 | `drivers/net/wireless/mediatek/mt76x2/` |
| Phase 5 | MT7612E 5GHz WiFi + PCIe 总线支持 | `drivers/net/wireless/mediatek/mt76x2/`, `drivers/pci/` |
| Phase 6 | HNAT (硬件 NAT) 兼容性适配 | `drivers/net/ethernet/raeth/` |

---

## 功能特性

### 核心路由功能

- **双频 WiFi**: 2.4GHz (802.11n) + 5GHz (802.11ac)，2x2 MIMO
- **IPv6**: 完整支持 (DHCPv6, SLAAC, NAT66)
- **Fullcone NAT**: 全锥形 NAT，改善 P2P 连通性
- **防火墙**: iptables/netfilter，支持 IP/MAC/URL 过滤
- **PPPoE**: 宽带拨号上网
- **DHCP/DNS**: dnsmasq 提供局域网域名解析和地址分配
- **UPnP**: NAT 穿透，支持游戏和 P2P 应用
- **VLAN**: 支持 VLAN 划分

### 管理与监控

- **Web 管理界面**: 中英文支持
- **实时流量监控**: 实时 / 24小时 / 每日统计
- **Wake-on-LAN**: 远程唤醒局域网设备
- **固件升级**: Web UI 一键升级
- **自定义脚本**: post_wan / post_iptables / inet_state / shutdown 钩子

### 硬件加速

- **HNAT (Hardware NAT)**: MT7620 硬件 NAT 加速，降低 CPU 占用，提升吞吐量
- **LED & GPIO**: 通过 sysfs 控制三色 LED (蓝/黄/红)

---

## 项目结构

```
padavan-4.4/
├── .github/workflows/       # GitHub Actions CI/CD
│   └── build.yml            #   自动化固件构建 (Ubuntu 22.04, 120min)
├── toolchain-mipsel/        # 交叉编译工具链 (crosstool-ng)
│   └── dl_toolchain.sh      #   工具链下载脚本
├── trunk/                   # 固件源码主目录
│   ├── linux-4.4.x/         #   Linux 4.4.198 内核源码
│   ├── user/                #   用户态程序 (18个组件)
│   │   ├── rc/              #     init/服务管理器 (核心)
│   │   ├── httpd/           #     Web 管理后台 (HTTP/HTTPS)
│   │   ├── www/             #     Web UI (ASP + Bootstrap)
│   │   ├── dnsmasq/         #     DNS/DHCP 服务器
│   │   ├── iptables/        #     防火墙 (iptables 1.8.7)
│   │   ├── pppd/ + pppoe/   #     PPP 拨号
│   │   ├── wpa_supplicant/  #     WiFi 认证 (WPA/WPA2)
│   │   ├── miniupnpd/       #     UPnP NAT 穿透
│   │   ├── iproute2/        #     高级网络工具
│   │   ├── busybox/         #     核心 Unix 工具
│   │   ├── libdisk/         #     USB 存储设备管理
│   │   ├── nvram/           #     NVRAM 读写库
│   │   ├── shared/          #     共享库 (nvram_linux, netutils, shutils)
│   │   ├── wireless_tools/  #     无线配置工具
│   │   ├── util-linux/      #     磁盘工具 (libblkid)
│   │   ├── utils/           #     硬件工具 (cpu_usage, hw_nat, switch)
│   │   └── scripts/         #     运行时脚本
│   ├── libs/                #   第三方库 (libz, libssl, libcurl, libevent 等)
│   ├── libc/                #   uClibc-ng 1.0.38
│   ├── configs/
│   │   ├── boards/MI-MINI/  #   硬件定义 (board.h, board.mk, kernel-4.4.x.config)
│   │   └── templates/       #   编译模板
│   ├── vendors/Ralink/      #   平台构建规则
│   ├── tools/               #   构建工具 (mksquashfs, mkimage 等)
│   ├── build_firmware_modify #  主编译脚本
│   └── Makefile             #   顶层构建系统
```

---

## 精简策略

相比上游 padavan-4.4，本项目进行了大幅精简，仅保留小米路由器迷你的核心路由功能：

| 移除类别 | 具体组件 |
|---------|---------|
| 非目标设备 | 所有非 MI-MINI 设备的板级支持 |
| VPN/代理 | OpenVPN, WireGuard, Shadowsocks, Trojan |
| 远程访问 | SSH (dropbear, OpenSSH) |
| 文件共享 | Samba, vsftpd, NFS |
| 媒体服务 | minidlna, firefly, xupnpd |
| 下载工具 | transmission, aria2 |
| 校园网认证 | dogcom, minieap, scutclient |
| QoS 调度 | IMQ/IFB 模块 |
| USB 扩展 | 摄像头/HID/串口/音频模块 |
| IPsec | XFRM 框架 |
| 组播代理 | igmpproxy (IPTV 组播) |
| 以太网桥过滤 | ebtables (访客 AP 隔离) |
| IP 集合 | ipset (高级 IP 过滤) |

精简后固件体积显著减小，运行时内存占用更低，更适合 MI-MINI 的 128MB RAM + 16MB Flash 硬件条件。

---

## 编译指南

### 环境要求

- **操作系统**: Debian/Ubuntu (推荐 Ubuntu 22.04)
- **磁盘空间**: 约 10GB (工具链 + 源码 + 构建产物)
- **内存**: 建议 4GB+

### 1. 安装依赖

```sh
sudo apt install unzip libtool-bin curl cmake gperf gawk flex bison nano xxd \
    fakeroot kmod cpio git python3-docutils gettext automake autopoint \
    texinfo build-essential help2man pkg-config zlib1g-dev libgmp3-dev \
    libmpc-dev libmpfr-dev libncurses5-dev libltdl-dev wget libc-dev-bin
```

### 2. 克隆源码

```sh
git clone https://github.com/Ben8368/padavan.git
cd padavan
```

### 3. 准备工具链

```sh
cd toolchain-mipsel
./dl_toolchain.sh
cd ..
```

### 4. 编译固件

```sh
cd trunk
fakeroot ./build_firmware_modify MI-MINI
```

编译产物位于 `trunk/images/` 目录。

### CI/CD

项目配置了 GitHub Actions 自动构建，支持以下触发方式：

| 触发方式 | 说明 |
|---------|------|
| **Push/PR** | 推送到 main 分支或提交 PR 时自动编译 |
| **手动触发** | 在 Actions 页面点击 "Run workflow" |
| **Star 触发** | 仓库 owner 点击 Star 按钮即可触发编译 |

工作流定义在 `.github/workflows/build.yml`，基于 Ubuntu 22.04 运行，超时时间 120 分钟，支持工具链缓存加速。编译成功后自动创建 GitHub Release 并上传 `.trx` 固件文件。

---

## AI 辅助开发

本项目是一个 **AI 驱动的嵌入式固件开发实践**。从零开始的 MT7620 内核移植、驱动调试、构建系统修复到 CI/CD 自动化，全部 43 个 commit 均在 AI 深度参与下完成。

### 使用的 AI 工具与模型

| 工具 | 模型 | 用途 |
|------|------|------|
| [Claude Code](https://claude.ai/code) | Claude Opus 4.7 | 内核源码分析、驱动移植方案设计、复杂 bug 根因排查 |
| [Claude Code](https://claude.ai/code) | Claude Sonnet 4.6 | 日常代码修改、构建脚本调试、文档生成 |

Claude Code 是 Anthropic 官方的 AI 编程助手，支持文件读写、Shell 执行、Agent 子任务并行、网页搜索等工具，能够直接操作代码仓库完成完整的开发工作流。

### AI 在各开发阶段的具体贡献

#### 1. 内核驱动移植 (6 个阶段，核心工作)

将 MT7620 平台支持从 rt-n56u 的旧内核移植到 padavan-4.4 的 Linux 4.4 内核，AI 负责：
- **源码分析**: 阅读并理解 rt-n56u 的 MT7620 驱动实现，识别需要移植的代码范围
- **DTS 编写**: 生成 MT7620 设备树节点，包括 USB PHY、PCIe、WiFi 等外设的地址和中断配置
- **驱动适配**: 分析 mt76x2 WiFi 驱动的 EEPROM 读取、PCIe 枚举、中断注册等流程，适配到新内核 API
- **HNAT 兼容**: 理解 hw_nat V2 的结构体差异，为 MT7620 定义正确的 Kconfig 符号和数据结构

#### 2. 驱动 Bug 修复 (11 个 commit)

AI 辅助分析编译错误和链接失败的根因，修复了多个跨模块问题：
- `hw_nat` 模块: 发现 `RALINK_MT7620` 符号未定义导致 HNAT_V2 结构体条件编译失败
- `mt76x2` 驱动: 定位 EEPROM 模式硬编码缺失、`SURFBOARDINT_WLAN` 宏未定义等隐含依赖
- `raeth` 驱动: 识别并清理未使用的函数，消除 -Werror 编译警告

#### 3. 构建系统修复 (6 个 commit)

AI 发现了一个隐蔽的宏不匹配 bug：
- **问题**: `USE_USB_SUPPORT` 和 `USE_STORAGE` 是两个独立宏，USB 代码依赖 libdisk 但构建系统只在存储启用时配置路径
- **排查**: AI 分析 Makefile 依赖链和条件编译逻辑，定位到构建顺序和头文件路径问题
- **修复**: 统一条件编译宏、调整 libdisk 在 rc 之前编译、添加缺失的 include 路径

#### 4. 固件精简与组件移除

AI 分析 20+ 个组件的依赖关系，安全移除不需要的功能：
- 识别每个组件在 rc、httpd、Makefile 中的调用点
- 对被移除组件的函数调用进行 stub 化处理，确保编译通过
- 精简后固件体积显著减小，适配 16MB Flash 限制

#### 5. CI/CD 自动化

AI 搭建完整的 GitHub Actions 工作流：
- 工具链缓存策略设计
- Star 触发机制（owner ID 检查防误触）
- 编译成功后自动创建 GitHub Release 并上传固件

### AI 辅助开发的效果

| 维度 | 说明 |
|------|------|
| **开发效率** | 内核驱动移植从零到可运行固件，43 个 commit 完成全部工作 |
| **问题排查** | AI 辅助分析内核源码中的隐含依赖和条件编译逻辑，减少试错成本 |
| **代码质量** | AI 审查驱动代码中的潜在问题，确保跨模块修改的一致性 |
| **知识门槛** | 降低了嵌入式 Linux 内核开发的入门门槛，无需逐行阅读数百万行内核源码 |

### Commit 中的 AI 签名

本项目的 commit message 中包含 `Co-Authored-By: Claude Opus 4.7` 签名，标识 AI 参与的贡献。项目文档和代码注释中也标注了 AI 辅助开发的说明。

---

## 更新日志

以下记录了从上游 [hanwckf/padavan-4.4](https://github.com/hanwckf/padavan-4.4) fork 后，针对小米路由器迷你 (MI-MINI, MT7620) 所做的全部修改。

### 项目精简

- 移除所有非 MI-MINI 设备的板级支持
- 精简固件组件：移除 VPN (OpenVPN/WireGuard/Shadowsocks)、SSH、Samba、vsftpd、NFS、minidlna、transmission、aria2、校园网认证、IPsec、igmpPoxy、ebtables、ipset 等
- 仅保留核心路由功能，适配 MI-MINI 的 128MB RAM + 16MB Flash

### 内核移植 (MT7620 平台)

| 阶段 | 内容 | 关键 commit |
|------|------|------------|
| Phase 1-2 | MT7620 SoC 基础支持，设备树 (DTS) 适配 | `9993e7a` |
| Phase 3 | USB PHY DTS 节点，USB 控制器初始化 | `16289df` |
| Phase 4 | MT7620 内置 2.4G WiFi 驱动适配 | `9f39a60` |
| Phase 5 | MT7612E 5GHz WiFi + PCIe 总线支持 | `ef74460` |
| Phase 6 | HNAT (硬件 NAT) 兼容性适配 | `d612bd0` |

### 内核 / 驱动修复

- **hw_nat**: 为 MT7620 定义 `RALINK_MT7620` Kconfig 符号，修复 HNAT_V2 结构体定义 (`d8ecc4b`, `06780f4`)
- **hw_nat**: 禁用 MT7621+ 专属的 `DFL_TTL0_DRP`，添加 MT7620 的 `CAH_RDATA` (`9672540`, `ae43b42`)
- **raeth**: 清理 `ra_switch.c` 和 `ra_dbg_proc.c` 中未使用的函数和变量 (`bb8a571`, `8c3845d`, `c084e36`)
- **mt76x2 驱动**: 定义 `CONFIG_RT_*_CARD_EEPROM`，硬编码 EEPROM 模式为 flash (`30534f4`, `8547024`)
- **mt76x2 驱动**: 添加 `SURFBOARDINT_WLAN` 定义，补充缺失符号的 stub 实现 (`2ad105c`, `1308bcb`)
- **mt76x2 驱动**: 包含 `mt7620.h` 获取 `SURFBOARDINT_WLAN` 定义 (`a191516`)

### 用户态 / 构建系统修复

- **httpd**: 修正 `ej_get_usb_ports_info` 声明和 stub 的条件编译宏，统一为 `USE_STORAGE` (`6de03a2`, `23995f7`)
- **rc (2026-05-20)**: 修复 USB-only 构建的链接错误 — 添加 libdisk include 路径给 `CONFIG_USB_SUPPORT`，调整构建顺序使 `libdisk` 在 `rc` 之前编译，守卫 `safe_remove_stor_device()` 调用 (`43b2178`, `c296042`, `e0d40bc`)
  - 根本原因：`USE_USB_SUPPORT` 和 `USE_STORAGE` 是两个独立宏，USB 代码依赖 libdisk 但构建系统之前只在存储启用时配置路径

### 组件精简 (2026-05-20)

- **ipset**: 关闭 `CONFIG_FIRMWARE_INCLUDE_IPSET`，删除源码目录（内核 ipset 模块本已未启用）
- **ebtables**: 从构建系统移除，stub 化 rc 中的 `ebtables_filter_guest_ap()` 和 `restart_guest_lan_isolation()`，关闭内核 ebtables 模块
- **igmpproxy**: 从构建系统移除，清理 rc 中所有 igmpproxy 调用（net.c, net_wan.c, rc.c, services.c, net_lan.c），简化 `restart_iptv()`

### CI/CD

- 添加 GitHub Actions 工作流，自动构建 MI-MINI 固件 (Ubuntu 22.04, 120min 超时, 工具链缓存) (`84dee98`)
- 构建失败时在 CI 输出中显示错误信息 (`adc6b9b`)
- 新增 Star 触发编译（仅 repo owner 生效）和自动发布 Release（`.trx` 固件上传到 GitHub Release）(`027d8f1`)

---

## 相关项目

- [hanwckf/padavan-4.4](https://github.com/hanwckf/padavan-4.4) — 上游项目
- [hanwckf/rt-n56u](https://github.com/hanwckf/rt-n56u) — MT7620 平台支持来源
- [Xiaomi MiMo](https://mimo.xiaomi.com) — 小米 AI 模型开放平台
- [Claude Code](https://claude.ai/code) — Anthropic AI 编程助手

## 许可证

本项目基于 [GNU General Public License v2](trunk/License) 开源。
