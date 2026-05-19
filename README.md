# padavan-4.4

基于 hanwckf/padavan-4.4 精简而来的路由器固件项目，目标设备为小米路由器迷你 (MI-MINI)。

使用 Linux 4.4.198 内核，从 rt-n56u 移植 MT7620 平台支持。

## 目标设备

| 设备 | SoC | WiFi | 备注 |
|------|-----|------|------|
| MI-MINI (小米路由器迷你) | MT7620 | 2.4G (内置) + 5G (MT7612E) | 唯一目标设备 |

## 特性

- Linux 4.4.198 内核 (MIPS32r2, mipsel, MT7620)
- 精简用户态，仅保留核心路由功能
- IPv6 支持
- Fullcone NAT
- LED & GPIO 控制 (sysfs)

## 项目结构

```
padavan-4.4/
├── toolchain-mipsel/        # 交叉编译工具链 (crosstool-ng)
├── trunk/
│   ├── linux-4.4.x/         # Linux 内核源码
│   ├── libs/                # 精简第三方库
│   ├── user/                # 精简用户态程序
│   │   ├── rc/              #   init/服务管理器
│   │   ├── httpd/           #   Web 管理后台
│   │   ├── dnsmasq/         #   DNS/DHCP
│   │   ├── iptables/        #   防火墙
│   │   ├── pppd/pppoe/      #   拨号
│   │   ├── wpa_supplicant/  #   WiFi
│   │   ├── www/             #   Web UI
│   │   └── ...
│   ├── configs/
│   │   ├── boards/          # 设备硬件定义
│   │   └── templates/       # 编译模板
│   ├── vendors/Ralink/      # 平台配置
│   ├── build_firmware_modify # 主编译脚本
│   └── Makefile             # 顶层构建
```

## 精简说明

相比上游 padavan-4.4，移除了：
- 所有非 MI-MINI 设备支持
- VPN/代理组件 (OpenVPN, WireGuard, Shadowsocks, Trojan 等)
- SSH (dropbear, openssh)
- 文件共享 (Samba, vsftpd, NFS)
- 媒体服务 (minidlna, firefly, xupnpd)
- BT 下载 (transmission, aria2)
- 校园网认证 (dogcom, minieap, scutclient 等)
- 其他非核心组件

## 编译步骤

### 1. 安装依赖

```sh
# Debian/Ubuntu
sudo apt install unzip libtool-bin curl cmake gperf gawk flex bison nano xxd \
    fakeroot kmod cpio git python3-docutils gettext automake autopoint \
    texinfo build-essential help2man pkg-config zlib1g-dev libgmp3-dev \
    libmpc-dev libmpfr-dev libncurses5-dev libltdl-dev wget libc-dev-bin
```

### 2. 克隆源码

```sh
git clone https://github.com/Ben8368/padavan.git
```

### 3. 准备工具链

```sh
cd padavan/toolchain-mipsel
./dl_toolchain.sh
```

### 4. 编译固件

```sh
cd padavan/trunk
fakeroot ./build_firmware_modify MI-MINI
```
