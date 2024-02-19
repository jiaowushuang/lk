# Newlk

#### 介绍
This is an operating system that is highly scalable, minimalist, and can be quickly commercialized for deployment(这是一个可扩展性极强，极简，可快速商业化部署的操作系统)

#### 软件架构
软件架构说明

构建系统：
1. cmake/make
2. kconfig - generate-time
3. dts - generate-time
4. generate file, xml, header file, cmake/make file(kconfig)
5. execute shell script
6. uboot/atf .etc

基础设施：
引导代码和常用库的正常编译和运行

OS架构：
该内核可以放在任意一级作为管理器，包括：
1. monitor - secureOS
2. hypervisor - manage VMs
3. superviser - manage APPs
4. administrator - a plugin for other OSs, and manage it
在任意层上，都可以使用某个Newlk来执行管理器的功能

可以复用相同的Newlk代码以减少存储，但不代表只要一份代码，而是多份Newlk的副本
所以，必不可少的两个问题需要考虑：
1. 当环境未建立时，需要初始化每一级的环境，包括全局和临时寄存器
2. 当环境建立后，全局寄存器可以不必更改，只需要对临时寄存器只需保存和恢复即可

务必使用最简洁的方式实现Newlk，其他都以库的形式作为可配置

传统上层级分立、金字塔式的镜像配置，例如el3 - atf, el2 - hypervisor, el1 - linux, el0 - app，变成扁平化的网状配置，例如Newlk-base -> Newlk-lib0 -> ... -> Newlk-libx, Newlk+

Newlk”粘性或高伸缩性“的原因在于，对上提供一个资源视图，其他用户接口标准（例如POSIX）都需要与资源视图进行适配，从而只要更新适配层，就可以为多种用户接口标准服务。
为了提供资源视图，需要对下提供更泛的资源概念，以及更泛的process概念，甚至用户可以自定义process，而放弃很有重量的thread/process，并且自定义的Process可以参与整个系统的Process调度上，这样在用户层也可以实现更泛的OS概念；这种更泛的process可以将内核层提供的各类接口（系统调用）”黏住“，从而在内核层之上提供一层抽象层，作为用户态OS；更泛的Process也可以兼容各类(superviser/hypervisor/monitor)OS功能以及各类硬件实现(arm/riscv/.etc)

Newlk别名学徒（apprentice），意为在学习中成长的内核

Newlk可以与UML(User Mode Linux)/LKL(Linux Kernel Library)/libos...进行合作，作为linux兼容层的一种解决方案；同时，也可以引入了Linux的一些库，包括锁以及基本数据结构，维测库，做成可以迁移的库
也可以参考embox方案


目前验证的配置（QEMU）：
HYPER   KERNEL 	USER
32	32	32
	64	64
	64	32

#### 安装教程
	make help
KCONFIG Examples:
	make DEFAULT_PROJECT=qemu-virt-arm64-test defconfig
	make DEFAULT_PROJECT=qemu-virt-arm64-test genconfig
	make DEFAULT_PROJECT=qemu-virt-arm64-test menuconfig
	make DEFAULT_PROJECT=qemu-virt-arm64-test dtbs
命令行
KERNEL
1.  ./scripts/do-qemuarm    :Cortex-A32
2.  ./scripts/do-qemuarm -3 :Cortex-M
3.  ./scripts/do-qemuarm -6 :Cortex-A64
4.  ./scripts/do-qemuarm -r :Cortex-R32
HYPER
1. -v

配置
1. make DEFAULT_PROJECT=qemu-virt-arm32[arm64|arm32r]-test menuconfig
2. env_inc.mk, WITH_SUPER_MODE := true
3. env_inc.mk, WITH_HYPER_MODE := true
4. USER_TASK_ENABLE := true
5. USER_TASK_32BITS := true
6. WITH_AUX_HYPER_MODE := true


HYPER - OK
32
命令行KERNEL 1|4，HYPER 1
配置1 DEFAULT_PROJECT=qemu-virt-arm32[arm32r]; cortex-r 配置mpu使能（partition number, multi-partition enable, multi-partition enable for RTOS, MPU ARM, MPU ARMV8R, ARM MPU Support），并且qemu需要改写成支持cortex-r52
配置3

AUX-HYPER - OK
32
命令行KERNEL 2
配置1 DEFAULT_PROJECT=qemu-virt-arm32, 配置mpu使能（partition number, multi-partition enable, multi-partition enable for RTOS, MPU ARM, ARM MPU Support）
配置6



KERNEL 	USER - ?
32	32
命令行KERNEL 1|2|4
配置1 DEFAULT_PROJECT=qemu-virt-arm32[arm32r]
配置2/4/5

64	64 - OK
命令行KERNEL 3
配置1 DEFAULT_PROJECT=qemu-virt-arm64
配置2/4

64	32 - OK
命令行KERNEL 3
配置1 DEFAULT_PROJECT=qemu-virt-arm64
配置2/4/5

#### 使用说明

1. 希望增加更多POSIX支持
2. 希望增加新的特性，例如中断虚拟化等
3. 希望兼容linux库
4. 希望WITH_ADMIN_MODE, WITH_MONITOR_MODE支持

#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request


#### 特技

1.  使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2.  Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)
3.  你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目
4.  [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目
5.  Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6.  Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)
