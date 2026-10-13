# Flow5Runner 构建记录

`Flow5Runner.exe` 使用本目录的 `Flow5Runner.cpp` 构建。当前目录中的 DLL 是与该 Debug 构建匹配的运行时文件。

构建依赖来自旧任务保留的工作区：

- `flow5lib-msvc-occt792`、`flow5io-msvc-occt792`
- `xfoil-msvc`、`gmsh-msvc-occt792`、`occt792-msvc`
- Qt 6.11 MSVC、Intel oneMKL 和 Visual Studio 18

重新生成时，使用 Visual Studio 生成器并把下面的路径替换为本机路径：

```powershell
cmake -S . -B ..\flow5runner-vsbuild -G "Visual Studio 18 2026" -A x64 `
  -DFLOW5_SOURCE_DIR=E:\development\flow5 `
  -DFLOW5_LIB_DIR=<flow5lib-msvc-occt792> `
  -DFLOW5_IO_LIB_DIR=<flow5io-msvc-occt792> `
  -DXFOIL_LIB_DIR=<xfoil-msvc> `
  -DGMSH_INCLUDE_DIR=E:\development\flow5\gmsh-4.15.2-source\api `
  -DGMSH_LIB_DIR=<gmsh-msvc-occt792> `
  -DQT_ROOT=D:\Qt\6.11.2\msvc2022_64 `
  -DOCCT_ROOT=<occt792-msvc> `
  -DMKL_ROOT="D:\Program Files (x86)\Intel\oneAPI\mkl\latest" `
  -DONEAPI_ROOT="D:\Program Files (x86)\Intel\oneAPI"
cmake --build ..\flow5runner-vsbuild --config Debug
```

本次已经用一个平面/极曲线 XML 对、单个攻角完成实际计算验证，返回码为 0，并生成了有效 CSV。
