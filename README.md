# WellsLk

#### Overview
The 'littlekernel/lk' fork for Well Safety Kernel.

##### Arch
This kernel can be placed at any level as a manager:
1. Monitor for Secure
2. Hypervisor for VM
3. Superviser for App
4. Administrator for a Plugin of other OS

Current progress:
- [x] Hypervisor
- [x] Superviser
- [ ] Monitor
- [ ] Administrator

A single code can be reused to create different project configurations.
Therefore, two essential issues need to be considered:
1. When the environment is not established, each level of the environment needs to be initialized, including global and temporary registers
2. When the environment is established, the global registers do not need to be changed, and only the temporary registers need to be saved and restored

We should refer to the microkernel architecture, where the kernel is streamlined and other functions are implemented in the form of libraries. From the perspective of image configuration, a kernel image (WellsLk) can be used with library images of different hardware levels to meet the resource management requirements of the project for all hardware levels (such as S/NS/Real World and corresponding exception levels in ARM).

WellsLk references a resource view framework as shown below:
![](doc/gf-framework.png)

Linux compatible:
1. UML(User Mode Linux)
2. LKL(Linux Kernel Library)
3. libos
4. embox

AIOS compatible:
1. LLM is used as the kernel and WellsLk is used as the LLM environment. WellsLk can provide an environment where multiple cores, heterogeneous cores, and multiple hosts are interconnected, and transactional tasks are deployed with transactional scheduling algorithms as the link. The advantage of transactional tasks is that it is relatively simple to reach consensus on shared objects, which is conducive to consensus combination and improves the efficiency of LLM interconnection. After all, shared data is the most convenient means of communication, and the strategies made by LLM can be easily converted into transactional tasks. The disadvantage of transactional scheduling is that it must ensure that there is always an optimal decision for each transaction, otherwise the system will be in chaos. The original scheduling decision has sufficient theoretical algorithm support, while LLM only brings experience after training.
2. LLM can collect data, make decisions, and feedback decisions. First, for OS with a large number of heuristic parameter configurations, such as Linux, it can greatly reduce the tuning cost. Secondly, different business scenarios have different strategies for scheduling/memory/IO/devices, such as hard real-time/soft real-time/non-real-time business. In future scenarios, the interaction and mixing of these scenarios are inevitable, which will inevitably cause the strategies applied in different scenarios to deviate greatly from the ideal situation. For example, when the hard real-time RT algorithm and the soft real-time CFS algorithm are interdependent, the wcet of the task in the RT algorithm and the vtime (priority increase) of the task in the CFS algorithm cannot be measured. Moreover, as the system runs for a long time, the deviation caused by this interdependence will accumulate, and the originally tuned parameters will age and become invalid. Transactional scheduling completely relies on the decision set summarized by LLM, and is not limited to the framework of the classic scheduling algorithm. It can make the best decision based on the input. In addition, transactional tasks are very easy to synchronize data (composability). In the face of sudden scenarios, transactional tasks are also very easy to recover, because multiple tasks classified as one transaction will hold a transaction state.
3. Traditional OS builds a hardware resource management platform in the form of data structure + deterministic algorithm + discrete components + local communication network, while AIOS builds a hardware resource management platform in the form of vector/matrix/tensor + non-deterministic algorithm + continuous components + global communication network.

AutoSAR compatible:
openAUTOSAR

##### BSP

Verified Platform and Manager:

Platform: QEMU

Manager:
|Hypervisor|Superviser|
|----|----|
|32|32(KM),32(UM)|
||64(KM),64(UM)|
||64(KM),32(UM)|

Enabled: We can modify the env_inc.mk file to configure different managers.
1. WITH_HYPER_MODE: Hypervisor
2. WITH_SUPER_MODE: Superviser
3. USER_TASK_ENABLE: UM
4. USER_TASK_32BITS: 32(UM)
5. WITH_AUX_HYPER_MODE: AUX-Hypervisor

#### Install

##### environment and dependencies
```
1. Ubuntu20.04/22.04
2. sudo apt install git binutils build-essential libssl-dev libncurses-dev libconfuse-dev libtool f2fs-tools device-tree-compiler python3 python3-dev python3-pip python-is-python3 gdb-multiarch
3. pip3 install --user ply jinja2 Kconfiglib
4. sudo apt install qemu-system-arm qemu-system-aarch64
5. qemu-system-arm for cortex-r52: Download QEMU(v8.2.0) source code and compilation, r52 support patch at 'external/qemu-cr52/cortex-r52-support.patch'
6. sudo apt install gcc-arm-none-eabi or [gcc-arm-none-eabi](https://developer.arm.com/downloads/-/gnu-a) and [aarch64-none-elf](https://developer.arm.com/downloads/-/gnu-a)
```

##### make with configuartion
We can execute `make help` to get all supported make commands.

Such as:
```
1. make DEFAULT_PROJECT=qemu-virt-arm32[arm64|arm32-r52]-test defconfig
2. make DEFAULT_PROJECT=qemu-virt-arm32[arm64|arm32-r52]-test genconfig
3. make DEFAULT_PROJECT=qemu-virt-arm32[arm64|arm32-r52]-test menuconfig
4. make DEFAULT_PROJECT=qemu-virt-arm32[arm64|arm32-r52]-test dtbs
```

##### compile and run

Superviser
```
1.  ./scripts/do-qemuarm    :Cortex-A32
2.  ./scripts/do-qemuarm -3 :Cortex-M
3.  ./scripts/do-qemuarm -6 :Cortex-A64
4.  ./scripts/do-qemuarm -r :Cortex-R32
```

Hypervisor
```
1. ./scripts/do-qemuarm -v    :Cortex-A32
2. ./scripts/do-qemuarm -r -v :Cortex-R32
```

##### steps

##### Hypervisor

For arm32-r52,
```
1. make DEFAULT_PROJECT=qemu-virt-arm32-r52-test defconfig
1.1 make DEFAULT_PROJECT=qemu-virt-arm32-r52-test menuconfig
1.2 Set True: partition number, multi-partition enable, multi-partition enable for RTOS, MPU ARM, MPU ARMV8R, ARM MPU Support
2. Set True: WITH_HYPER_MODE
3. ./scripts/do-qemuarm -r -v
```

For arm32-a,
```
1. make DEFAULT_PROJECT=qemu-virt-arm32-test defconfig
2. Set True: WITH_HYPER_MODE
3. ./scripts/do-qemuarm -v
```

##### AUX-Hypervisor 
```
For arm32-a,
1. make DEFAULT_PROJECT=qemu-virt-arm32-test defconfig
1.1 Set True: partition number, multi-partition enable, multi-partition enable for RTOS, MPU ARM, ARM MPU Support
2. Set True: WITH_AUX_HYPER_MODE
3. ./scripts/do-qemuarm -3
```

##### Superviser: 32(KM), 32(UM)
```
For cortex-a/r/m,
1. make DEFAULT_PROJECT=qemu-virt-arm32-test defconfig
For cortex-m, We can choose whether to use MPU. MPU Default Configuration: partition number, multi-partition enable, multi-partition enable for APP, MPU ARM, ARM MPU Support.
When disabling MPU, Set false: ENABLE_MPU(in arch/arm/rules.mk).
For coretx-r, We only support R52 and MPU must be enabled. MPU Default Configuration: partition number, multi-partition enable, multi-partition enable for APP, MPU ARM, MPU ARMV8R, ARM MPU Support.
2. Set True: WITH_SUPER_MODE/USER_TASK_ENABLE/USER_TASK_32BITS
For cortex-a,
3. ./scripts/do-qemuarm
For cortex-r,
3. ./scripts/do-qemuarm -r
For cortex-m,
3. ./scripts/do-qemuarm -3
```

##### Superviser: 64(KM), 64(UM)
```
1. make DEFAULT_PROJECT=qemu-virt-arm64-test defconfig
2. Set True: WITH_SUPER_MODE/USER_TASK_ENABLE
3. ./scripts/do-qemuarm -6
```
##### Superviser: 64(KM), 32(UM)
```
1. make DEFAULT_PROJECT=qemu-virt-arm64-test defconfig
2. Set True: WITH_SUPER_MODE/USER_TASK_ENABLE/USER_TASK_32BITS
3. ./scripts/do-qemuarm -6
```

##### debug and run

Superviser
```
1.  ./scripts/do-qemuarm    -b :Cortex-A32 <br>
2.  ./scripts/do-qemuarm -3 -b :Cortex-M <br>
3.  ./scripts/do-qemuarm -6 -b :Cortex-A64 <br>
4.  ./scripts/do-qemuarm -r -b :Cortex-R32 <br>
```

Hypervisor
```
1. ./scripts/do-qemuarm -v -b    :Cortex-A32 <br>
2. ./scripts/do-qemuarm -r -v -b :Cortex-R32 <br>
```

run
```
1. ./scripts/do-qemuarm-d3  :Cortex-M, Cortex-A32 <br>
2. ./scripts/do-qemuarm-d6  :Cortex-A64 <br>
3. ./scripts/do-qemuarm-dr3 :Cortex-R32 <br>
```

