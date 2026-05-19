# padavan-4.4

基于原版 RT-N56U 的第三方路由器固件项目，使用 MTK 官方 Linux 4.4.198 内核（源自 D-LINK GPL 代码）。

## 特性

- Linux 4.4.198 内核 (MIPS32r2, mipsel)
- 支持 MT7621 SoC 平台
- 支持 MT7615D/N、MT7915D 无线芯片 (Wi-Fi 5/6)
- 硬件 NAT (HNAT) + 软件快速转发 (SFE) 双加速
- IPv6 NAT (基于 netfilter)
- WireGuard 内核集成
- Fullcone NAT (by Chion82)
- LED & GPIO 控制 (sysfs)

## 项目结构

```
padavan-4.4/
├── toolchain-mipsel/        # 交叉编译工具链 (crosstool-ng)
├── trunk/
│   ├── linux-4.4.x/         # Linux 内核源码
│   ├── libs/                # 第三方库 (openssl, libcurl, mbedtls 等)
│   ├── user/                # 用户态程序 (70+ 组件)
│   │   ├── rc/              #   init/服务管理器
│   │   ├── httpd/           #   Web 管理后台
│   │   ├── dnsmasq/         #   DNS/DHCP
│   │   ├── iptables/        #   防火墙
│   │   ├── openvpn/         #   OpenVPN
│   │   ├── wireguard-tools/ #   WireGuard
│   │   ├── shadowsocks/     #   SS/SSR
│   │   ├── trojan/          #   Trojan
│   │   ├── samba36/         #   文件共享
│   │   ├── transmission/    #   BT 下载
│   │   ├── www/             #   Web UI 前端 (~110 个页面)
│   │   └── ...
│   ├── configs/
│   │   ├── boards/          # 设备硬件定义 (board.h / board.mk)
│   │   └── templates/       # 编译模板 (.config)
│   ├── vendors/Ralink/      # 平台配置
│   ├── build_firmware_modify # 主编译脚本
│   └── Makefile             # 顶层构建
```

## 支持设备

| 设备 | 品牌 | 备注 |
|------|------|------|
| K2P / K2P-USB | 斐讯 | 经典机型 |
| CR660x | 小米 | |
| DIR-878 / DIR-882 | D-Link | |
| JCG-Q20 / JCG-AC860M / JCG-836PRO / JCG-Y2 | 捷稀 | |
| MI-R3P | 小米 | |
| NETGEAR-BZV | Netgear | |
| MR2600 | 摩托罗拉 | |
| XY-C1 | | |

## VPN / 代理组件

- **VPN**: OpenVPN, WireGuard, StrongSwan (IPsec), SoftEther VPN
- **代理**: Shadowsocks/SSR, Trojan, xTun, Redsocks, ipt2socks
- **内网穿透**: frp
- **校园网认证**: dogcom, minieap, njit-client, scutclient, mentohust

## 编译步骤

### 1. 安装依赖

```sh
# Debian/Ubuntu
sudo apt install unzip libtool-bin curl cmake gperf gawk flex bison nano xxd \
    fakeroot kmod cpio git python3-docutils gettext automake autopoint \
    texinfo build-essential help2man pkg-config zlib1g-dev libgmp3-dev \
    libmpc-dev libmpfr-dev libncurses5-dev libltdl-dev wget libc-dev-bin

# Archlinux/Manjaro
sudo pacman -Syu --needed git base-devel cmake gperf ncurses libmpc \
        gmp python-docutils vim rpcsvc-proto fakeroot cpio help2man

# Alpine
sudo apk add make gcc g++ cpio curl wget nano xxd kmod \
    pkgconfig rpcgen fakeroot ncurses bash patch \
    bsd-compat-headers python2 python3 zlib-dev \
    automake gettext gettext-dev autoconf bison \
    flex coreutils cmake git libtool gawk sudo
```

### 2. 克隆源码

```sh
git clone https://github.com/Ben8368/padavan.git
```

### 3. 准备工具链

```sh
cd padavan/toolchain-mipsel

# (推荐) 下载预编译工具链 (x86_64 或 aarch64)
./dl_toolchain.sh

# 或使用 crosstool-ng 自行编译
# ./build_toolchain
```

### 4. 编译固件

```sh
cd padavan/trunk

# (可选) 修改模板配置
# nano configs/templates/K2P.config

# 开始编译
fakeroot ./build_firmware_modify K2P

# 编译其他设备前先清理
./clear_tree
```

## 参考资料

- 控制 GPIO 和 LED (sysfs)
- 使用 NAND RWFS 分区
- 使用 IPv6 NAT 和 Fullcone NAT
- 通过设备树添加新设备支持
