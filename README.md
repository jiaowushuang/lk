# gfGuest

#### 介绍
这是一个可扩展性极强，极简，可快速商业化部署（注，该代码仅作为原型阶段的产物，需结合业务需求进行完善，而非直接产品化使用）的操作系统。

#### 软件架构

##### 构建系统：
1. make
2. kconfig - generate-time
3. dts - generate-time
4. generate file, xml, header file, cmake/make file(kconfig)
5. execute shell script
6. uboot/atf .etc

##### 基础设施：
引导代码和常用库的正常编译和运行

##### OS架构：
该内核可以放在任意一级作为管理器，包括：
1. monitor - secureOS
2. hypervisor - manage VMs
3. superviser - manage APPs
4. administrator - a plugin for other OSs, and manage it
在任意层上，都可以使用某个gfGuest来执行管理器的功能
> 目前hypervisor和superviser已实现，administrator在筹备中

可以复用相同的gfGuest代码以减少存储，但不代表只要一份代码，而是多份gfGuest的副本
所以，必不可少的两个问题需要考虑：
1. 当环境未建立时，需要初始化每一级的环境，包括全局和临时寄存器
2. 当环境建立后，全局寄存器可以不必更改，只需要对临时寄存器只需保存和恢复即可

务必使用最简洁的方式实现gfGuest，其他都以库的形式作为可配置

传统上层级分立、金字塔式的镜像配置，例如el3 - atf, el2 - hypervisor, el1 - linux, el0 - app，变成扁平化的网状配置，例如gfGuest-base -> gfGuest-lib0 -> ... -> gfGuest-libx, gfGuest+

gfGuest”粘性或高伸缩性“的原因在于，对上提供一个资源视图，其他用户接口标准（例如POSIX）都需要与资源视图进行适配，从而只要更新适配层，就可以为多种用户接口标准服务。
为了提供资源视图，需要对下提供更泛的资源概念，以及更泛的process概念，甚至用户可以自定义process，而放弃很有重量的thread/process，并且自定义的Process可以参与整个系统的Process调度上，这样在用户层也可以实现更泛的OS概念；这种更泛的process可以将内核层提供的各类接口（系统调用）”黏住“，从而在内核层之上提供一层抽象层，作为用户态OS；更泛的Process也可以兼容各类(superviser/hypervisor/monitor)OS功能以及各类硬件实现(arm/riscv/.etc)

gfGuest别名学徒（apprentice），意为在学习中成长的内核

##### 兼容Linux方案：
gfGuest可以与UML(User Mode Linux)/LKL(Linux Kernel Library)/libos...进行合作，作为linux兼容层的一种解决方案；同时，也可以引入了Linux的一些库，包括锁以及基本数据结构，维测库，做成可以迁移的库
也可以参考embox

##### AIOS方案：
以LLM作为内核，gfGuest作为LLM实现，gfGuest可以分布在多核，异构核，多主机，彼此互联，并以事务性调度算法为纽带，部署事务性任务，事务性任务好处在于对于共享对象的共识性达成较为简单，这有利于共识性的组合，同时提高LLM互联效率，毕竟共享数据是最为便捷的通信手段，并且LLM做出的策略可以很容易转换为事务性任务。事务性调度劣势在于，必须保证有一次事务都是最优决策，否则系统将混乱不堪，原有的调度决策有足够的理论算法支撑，而LLM带来的只有训练后的经验。

LLM可以收集数据，制定决策，反馈决策，这首先对于具备大量启发式参数配置的OS，例如Linux，可以极大的降低调优成本，其次，不同业务场景下，对于调度/内存/IO/设备等策略都有所不同，例如硬实时/软实时/非实时业务等，在未来场景中，这些场景的交互和混合是不可避免的，那么势必造成，在不同场景应用的策略与理想情况出现较大偏差，例如，硬实时的rt算法与软实时的cfs算法，在二者任务相互依赖时，rt算法中任务的wcet，与cfs算法任务的vtime(优先级提升)都不可测定，并且，随着系统运行时间推移，这种相互依赖产生的偏差会累积，原本调优的参数都会老化和失效。而事务性调度完全依赖于LLM归纳的决策集，不拘泥经典调度算法的框架，可以根据输入做出最优决策。并且，事务性的任务非常易于数据同步（可组合性）。面对突发场景，事务性任务也非常易于恢复，因为划为一个事务的多个任务，会持有一个事务状态。

LLM全局环境可以采用物理环境或是虚拟环境，CPU或是虚拟机virt（例如bpf），RTOS(LLM as CPU)，Linux(LLM as virt)。主要原因在于，RTOS代码较为简单，应用场景简单，所以直接不用过于在于其生态，而像Linux，已经建立足够强大的生态，只能退而控制Linux决策，而不是放弃Linux。

1. rust 作为 LLM 开发 首选语言
2. rust+c 作为内核开发首选语言

AIOS在内核态的实现上要扭转一个观念：传统OS是以数据结构+确定性算法+离散组件+局部通信网络的形式来搭建一个硬件资源管理平台，而AIOS是以向量/矩阵/张量+非确定性算法+连续组件+全局通信网络的形式

##### 兼容AutoSAR方案：
参考openAUTOSAR


##### 目前验证的配置（QEMU）：

|HYPER支持位数|KERNEL支持位数|USER支持位数|
|----|----|----|
|32|32|32|
||64|64|
||64|32|

#### 安装教程

##### 安装环境以及依赖：
1. Ubuntu20.04/22.04
2. 安装通用库：sudo apt install git binutils build-essential libssl-dev libncurses-dev libconfuse-dev libtool f2fs-tools device-tree-compiler python3 python3-dev python3-pip python-is-python3
3. 安装py库：pip3 install --user ply jinja2 Kconfiglib
4. 安装QEMU：sudo apt install qemu-system-arm qemu-system-aarch64 *qemu-system-arm for cortex-r52 需要更新QEMU代码为v8.2.0，源码编译，patch at 'external/qemu-cr52/cortex-r52-support.patch'*
5. 安装交叉编译器(arm32)：sudo apt install gcc-arm-none-eabi 或是 [gcc-arm-none-eabi](https://developer.arm.com/downloads/-/gnu-a)
6. 安装交叉编译器(arm64)：[aarch64-none-elf](https://developer.arm.com/downloads/-/gnu-a)

##### 获取make帮助：
根据硬件平台，在一个终端窗口输入以下命令：
1. make help <br>
2. 针对Kconfig的配置样例: <br>
2.1 make DEFAULT_PROJECT=qemu-virt-arm32[arm64|arm32-r52]-test defconfig <br>
2.2 make DEFAULT_PROJECT=qemu-virt-arm32[arm64|arm32-r52]-test genconfig <br>
2.3 make DEFAULT_PROJECT=qemu-virt-arm32[arm64|arm32-r52]-test menuconfig <br>
2.4 make DEFAULT_PROJECT=qemu-virt-arm32[arm64|arm32-r52]-test dtbs <br>

##### 编译命令行汇总 <br>
根据硬件平台，在一个终端窗口输入以下命令：<br>
KERNEL <br>
1.  ./scripts/do-qemuarm    :Cortex-A32 <br>
2.  ./scripts/do-qemuarm -3 :Cortex-M <br>
3.  ./scripts/do-qemuarm -6 :Cortex-A64 <br>
4.  ./scripts/do-qemuarm -r :Cortex-R32 <br>

HYPER <br>
1. ./scripts/do-qemuarm -v    :Cortex-A32 <br>
2. ./scripts/do-qemuarm -r -v :Cortex-R32 <br>

##### Kconfig/Makefile配置汇总
根据硬件平台，在一个终端窗口输入以下命令： <br>
1. make DEFAULT_PROJECT=qemu-virt-arm32[arm64|arm32-r52]-test menuconfig
2. env_inc.mk文件中, 修改Make变量值, WITH_SUPER_MODE := true
3. env_inc.mk文件中, 修改Make变量值, WITH_HYPER_MODE := true
4. env_inc.mk文件中, 修改Make变量值, USER_TASK_ENABLE := true
5. env_inc.mk文件中, 修改Make变量值, USER_TASK_32BITS := true
6. env_inc.mk文件中, 修改Make变量值, WITH_AUX_HYPER_MODE := true

> 注意!!! 所有验证需要按照案例的步骤执行

##### HYPER案例 - 验证OK 
支持32位 <br>
1. 选择Kconfig/Makefile配置汇总1, 即*DEFAULT_PROJECT=qemu-virt-arm32[arm32-r52]*, 其中如果是cortex-a，不需要特别设置Kconfig；如果硬件平台是cortex-r，需要配置mpu使能 Kconfig（partition number, multi-partition enable, multi-partition enable for RTOS, MPU ARM, MPU ARMV8R, ARM MPU Support），并且qemu需要改写成支持cortex-r52 <br>
2. 选择Kconfig/Makefile配置汇总3, 即 *WITH_HYPER_MODE := true* <br>
3. 选择编译命令行汇总 HYPER 1|2, 即根据硬件平台选择 *./scripts/do-qemuarm -v* 或 *./scripts/do-qemuarm -r -v* <br>

##### AUX-HYPER案例 - 验证OK 
支持32位 <br>
1. 选择Kconfig/Makefile配置汇总1, 即*DEFAULT_PROJECT=qemu-virt-arm32*, 配置mpu使能 Kconfig（partition number, multi-partition enable, multi-partition enable for RTOS, MPU ARM, ARM MPU Support） <br>
2. 选择Kconfig/Makefile配置汇总6, 即 *WITH_AUX_HYPER_MODE := true* <br>
3. 选择编译命令行汇总 KERNEL 2, 即 *./scripts/do-qemuarm -3* <br>

##### KERNEL-USER案例1 - 验证OK 
支持K32-U32位 <br>
1. 选择Kconfig/Makefile配置汇总1, 即*DEFAULT_PROJECT=qemu-virt-arm32* <br>
2. 如果是cortex-m 默认需要配置mpu使能 Kconfig（partition number, multi-partition enable, multi-partition enable for APP, MPU ARM, ARM MPU Support）<br>
2.1 当不使用MPU时, 需要在arch/arm/rules.mk中配置 *ENABLE_MPU:=false* <br>
2.2 cortex-m目前仅支持KERNEL32, 使用配置2+4+5, 即*WITH_SUPER_MODE := true;USER_TASK_ENABLE := true;USER_TASK_32BITS := true* <br>
3. 如果是coretx-r 默认需要配置mpu使能 Kconfig（partition number, multi-partition enable, multi-partition enable for APP, MPU ARM, MPU ARMV8R, ARM MPU Support） <br>
3.1 cortex-r目前仅支持KERNEL32, 使用配置2+4+5, 即*WITH_SUPER_MODE := true;USER_TASK_ENABLE := true;USER_TASK_32BITS := true* <br>
3.2 cortex-r目前仅验证r52, 且必须配置mpu使能 <br>
4. 选择编译命令行汇总 KERNEL 1|2|4, 即根据硬件平台选择 *./scripts/do-qemuarm, ./scripts/do-qemuarm -3, ./scripts/do-qemuarm -r* <br>

##### KERNEL-USER案例2 - 验证OK
支持K64-U64位
1. 选择Kconfig/Makefile配置汇总1, 即*DEFAULT_PROJECT=qemu-virt-arm64* <br>
2. 选择Kconfig/Makefile配置汇总2+4, 即*WITH_SUPER_MODE := true;USER_TASK_ENABLE := true* <br>
3. 选择编译命令行汇总KERNEL 3, 即 *./scripts/do-qemuarm -6* <br>

##### KERNEL-USER案例3 - 验证OK 
支持K64-U32位
1. 选择Kconfig/Makefile配置汇总1, *DEFAULT_PROJECT=qemu-virt-arm64* <br>
2. 选择Kconfig/Makefile配置汇总2+4+5, 即*WITH_SUPER_MODE := true;USER_TASK_ENABLE := true;USER_TASK_32BITS := true* <br>
3. 选择编译命令行汇总KERNEL 3, 即 *./scripts/do-qemuarm -6* <br>

##### 内核调试
gdb安装：sudo apt install gdb-multiarch <br>
##### 编译命令行汇总 <br>
根据硬件平台，在一个终端窗口输入以下命令：<br>
KERNEL <br>
1.  ./scripts/do-qemuarm    -b :Cortex-A32 <br>
2.  ./scripts/do-qemuarm -3 -b :Cortex-M <br>
3.  ./scripts/do-qemuarm -6 -b :Cortex-A64 <br>
4.  ./scripts/do-qemuarm -r -b :Cortex-R32 <br>

HYPER <br>
1. ./scripts/do-qemuarm -v -b    :Cortex-A32 <br>
2. ./scripts/do-qemuarm -r -v -b :Cortex-R32 <br>

##### 调试命令行汇总
根据硬件平台，在另一个终端窗口输入以下命令：<br>
1. ./scripts/do-qemuarm-d3  :Cortex-M, Cortex-A32 <br>
2. ./scripts/do-qemuarm-d6  :Cortex-A64 <br>
3. ./scripts/do-qemuarm-dr3 :Cortex-R32 <br>
   
#### 使用说明

1. 希望增加更多POSIX或其他接口标准（AutoSAR）支持，建议参考embox兼容层/openAUTOSAR设计
2. 希望增加新的特性，例如中断虚拟化等，建议参考Xen设计
3. 希望兼容linux库，建议参考embox或其他设计
4. 希望WITH_ADMIN_MODE支持, 参考AIOS方案
5. 希望WITH_MONITOR_MODE支持，参考ATF设计
