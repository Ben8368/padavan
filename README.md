# padavan-4.4 — 小米路由器迷你 (MI-MINI) 定制固件

基于 [hanwckf/padavan-4.4](https://github.com/hanwckf/padavan-4.4) 精简并定制的路由器固件项目，专门针对 **小米路由器迷你 (MI-MINI)** 进行 MT7620 平台移植。使用 Linux 4.4.198 内核，参考 rt-n56u 的 MT7620 支持完成驱动适配与硬件 NAT 兼容性修改。

> 本项目开发过程中使用了 **AI 辅助开发** (Claude Code)，在内核移植、驱动调试、CI/CD 搭建等环节借助 AI 提升效率。

---

## 项目亮点

| 维度 | 说明 |
|------|------|
| **目标设备** | 小米路由器迷你 (MI-MINI)，MT7620 SoC，128MB RAM |
| **内核版本** | Linux 4.4.198，MIPS32r2 架构 (mipsel) |
| **WiFi** | 双频：2.4G (MT7620 内置) + 5G 802.11ac (MT7612E PCIe) |
| **开发方式** | AI 辅助开发 (Claude Code)，分阶段迭代移植 |
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
- **防火墙**: iptables/netfilter + IPSet，支持 IP/MAC/URL 过滤
- **PPPoE**: 宽带拨号上网
- **DHCP/DNS**: dnsmasq 提供局域网域名解析和地址分配
- **UPnP**: NAT 穿透，支持游戏和 P2P 应用
- **IGMP Proxy**: IPTV 组播代理
- **VLAN**: 支持 IPTV VLAN 划分

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

项目配置了 GitHub Actions 自动构建，每次推送自动触发固件编译。工作流定义在 `.github/workflows/build.yml`，基于 Ubuntu 22.04 运行，超时时间 120 分钟，支持工具链缓存加速。

---

## AI 辅助开发说明

本项目在开发过程中使用了 AI 工具 (Claude Code) 辅助以下工作：

- **内核驱动移植**: MT7620 WiFi 驱动适配、PCIe 总线配置、HNAT 兼容性修改时，借助 AI 分析内核源码和排查问题
- **代码审查**: 辅助检查内核配置和驱动代码中的潜在问题
- **CI/CD 搭建**: GitHub Actions 工作流的配置
- **文档编写**: 项目文档的撰写

在嵌入式固件开发中，AI 工具在代码理解和问题排查环节能提供一定帮助，尤其适合需要大量参考内核源码的跨平台移植场景。

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

---

## 相关项目

- [hanwckf/padavan-4.4](https://github.com/hanwckf/padavan-4.4) — 上游项目
- [hanwckf/rt-n56u](https://github.com/hanwckf/rt-n56u) — MT7620 平台支持来源
- [Xiaomi MiMo](https://mimo.xiaomi.com) — 小米 AI 模型开放平台
- [Claude Code](https://claude.ai/code) — Anthropic AI 编程助手

## 许可证

本项目基于 [GNU General Public License v2](trunk/License) 开源。
