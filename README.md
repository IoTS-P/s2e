S2E Library
===========

This repository contains all the necessary components to build ``libs2e.so``. This shared
library is preloaded in QEMU to enable symbolic execution.

Please refer to the documentation in the ``docs`` directory for build and usage instructions.
You can also find it online on <https://s2e.systems/docs>.


## 修改后如何编译并运行
为编译后的 `libs2e-release` 中的 `Makefile` 文件加入了 `install` 命令。
帮助将编译好的 `so/arm` 链接文件等安装在对应的目录：

```bash
# 在 $uEumuDIR/build/libs2e-release 文件夹下
make 
make install 
```
然后不用重新利用 `uEmu-helper.py` 生成文件，直接执行生成的`sh`文件即可