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

AIOS方案：
以LLM作为内核，newlk作为LLM实现，newlk可以分布在多核，异构核，多主机，彼此互联，并以事务性调度算法为纽带，部署事务性任务，事务性任务好处在于对于共享对象的共识性达成较为简单，这有利于共识性的组合，同时提高LLM互联效率，毕竟共享数据是最为便捷的通信手段，并且LLM做出的策略可以很容易转换为事务性任务。事务性调度劣势在于，必须保证有一次事务都是最优决策，否则系统将混乱不堪，原有的调度决策有足够的理论算法支撑，而LLM带来的只有训练后的经验。

LLM可以收集数据，制定决策，反馈决策，这首先对于具备大量启发式参数配置的OS，例如Linux，可以极大的降低调优成本，其次，不同业务场景下，对于调度/内存/IO/设备等策略都有所不同，例如硬实时/软实时/非实时业务等，在未来场景中，这些场景的交互和混合是不可避免的，那么势必造成，在不同场景应用的策略与理想情况出现较大偏差，例如，硬实时的rt算法与软实时的cfs算法，在二者任务相互依赖时，rt算法中任务的wcet，与cfs算法任务的vtime(优先级提升)都不可测定，并且，随着系统运行时间推移，这种相互依赖产生的偏差会累积，原本调优的参数都会老化和失效。而事务性调度完全依赖于LLM归纳的决策集，不拘泥经典调度算法的框架，可以根据输入做出最优决策。并且，事务性的任务非常易于数据同步（可组合性）。面对突发场景，事务性任务也非常易于恢复，因为划为一个事务的多个任务，会持有一个事务状态。

LLM全局环境可以采用物理环境或是虚拟环境，CPU或是虚拟机virt（例如bpf），RTOS<LLM as CPU>，Linux<LLM as virt。主要原因在于，RTOS代码较为简单，应用场景简单，所以直接不用过于在于其生态，而像Linux，已经建立足够强大的生态，只能退而控制Linux决策，而不是放弃Linux。



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
1. make DEFAULT_PROJECT=qemu-virt-arm32[arm64|arm32-r52]-test menuconfig
2. env_inc.mk, WITH_SUPER_MODE := true
3. env_inc.mk, WITH_HYPER_MODE := true
4. USER_TASK_ENABLE := true
5. USER_TASK_32BITS := true
6. WITH_AUX_HYPER_MODE := true


HYPER - OK
32
命令行KERNEL 1|4，HYPER 1
配置1 DEFAULT_PROJECT=qemu-virt-arm32[arm32-r52]; cortex-r 配置mpu使能 Kconfig（partition number, multi-partition enable, multi-partition enable for RTOS, MPU ARM, MPU ARMV8R, ARM MPU Support），并且qemu需要改写成支持cortex-r52
配置3

AUX-HYPER - OK
32
命令行KERNEL 2
配置1 DEFAULT_PROJECT=qemu-virt-arm32, 配置mpu使能 Kconfig（partition number, multi-partition enable, multi-partition enable for RTOS, MPU ARM, ARM MPU Support）
配置6

KERNEL 	USER - OK
32	32
命令行KERNEL 1|2|4
配置1 DEFAULT_PROJECT=qemu-virt-arm32
配置2/4/5
cortex-m 默认配置mpu使能 Kconfig（partition number, multi-partition enable, multi-partition enable for APP, MPU ARM, ARM MPU Support）
当不使用MPU时，需要在arch/arm/rules.mk中配置ENABLE_MPU:=false
cortex-m目前仅支持KERNEL32，使用配置2
cortex-r目前仅验证r52，且必须配置mpu使能
默认配置mpu使能 Kconfig（partition number, multi-partition enable, multi-partition enable for APP, MPU ARM, MPU ARMV8R, ARM MPU Support）
cortex-r目前仅支持KERNEL32，使用配置2

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
