# CY8C4149AZI-S595 分区触摸固件

面向 Curva 手台的 34 通道分区电容触摸驱动板固件，适配 **CY8C4149AZI-S595 / CY8C4149AZQ-S595** 主控。使用 CAPSENSE 中间件扫描 ITO 触摸区域，并通过 UART 向主控板持续发送各通道的触摸采样数据。

对应的主控板固件：[QHPaeek/Mai_stm32](https://github.com/QHPaeek/Mai_stm32)。

## 基础配置

- 开发环境：ModusToolbox + VSCode / ModusToolbox Assistant。
- 硬件供电及 UART 电平：3.3 V。
- 触摸采样：CSD 自电容，MSC0 / MSC1 分别负责 19 / 15 个通道，在 19 个 slot 中并行扫描。
- UART：SCB4，222222_8N1

## GPIO 分配

| 功能 | GPIO | 说明 |
| --- | --- | --- |
| UART TX | P4.5 | 连接主控板 RX |
| UART RX | P4.4 |   |
| LED | P3.4 | 低电平点亮，每轮循环翻转 |
| SWDIO | P3.2 | SWD 调试 |
| SWCLK | P3.3 | SWD 调试 |
| 复位 | XRES |   |
| MSC0 CMOD1 / CMOD2 | P4.0 / P4.1 |   |
| MSC1 CMOD1 / CMOD2 | P7.0 / P7.1 |   |

触摸通道与 GPIO 的对应关系如下，连续范围按顺序一一对应：

| 触摸通道 | GPIO | 扫描单元 |
| --- | --- | --- |
| CH0～CH7 | P1.0～P1.7 | MSC0 |
| CH8～CH15 | P2.0～P2.7 | MSC0 |
| CH16 | P6.0 | MSC0 |
| CH17～CH18 | P4.6～P4.7 | MSC0 |
| CH19～CH20 | P5.6～P5.7 | MSC1 |
| CH21～CH28 | P0.0～P0.7 | MSC1 |
| CH29～CH32 | P5.0～P5.3 | MSC1 |
| CH33 | P5.5 | MSC1 |

触摸通道编号对应 `Button0`～`Button33`，实际区域映射由主控板 / 上位机配置。

## 串口数据帧

每帧固定 70 字节：

| 字节偏移 | 内容 |
| --- | --- |
| 0 | 起始字节 `0x00` |
| 1～68 | CH0～CH33，每通道 16 位无符号数，低字节在前 |
| 69 | 本帧 68 字节通道数据的累加和，取低 8 位 |

当前发送的是按各通道 `maxRawCount` 归一化后的 raw 值，而非触摸状态或 `raw - baseline`。缩放先映射到 `0～4095`，再左移 4 位，输出范围为 `0～65520`；初始化时预计算定点增益以减少循环中的除法开销。

扫描、处理和 UART 非阻塞发送采用双缓冲流水线。帧率取决于触摸负载和自动调校后的扫描参数，不设固定发送周期。

## 构建与调试

安装 ModusToolbox 和 GCC ARM 工具链，在工程根目录的 ModusToolbox 终端执行：

```bash
make getlibs
make vscode
make build CONFIG=Debug
```

通过 VSCode 打开 `cy8c4149-s595-34ch-cs5-curvatouch.code-workspace`，调试配置使用 KitProg3。

硬件资源与 CAPSENSE 参数代码通过 Device Configurator / CAPSENSE Configurator 修改并生成，配置文件位于 `bsps/TARGET_CURVA_CY8C4149_S595/config/` 下的 `design.modus` 和 `design.cycapsense`。
