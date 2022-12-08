# IFR OpenCV Frame

IFR实验室视觉代码框架

前端部分: [https://github.com/MineYuanlu/ifr-opencv-web](https://github.com/MineYuanlu/ifr-opencv-web)

## BASIC

- GCC 14
- OpenCV 4
- 大恒相机驱动
- 串口API
- [rapidjson](https://github.com/Tencent/rapidjson)
- [mongoose](https://github.com/cesanta/mongoose)
- [ifr-modules](https://github.com/mineYuanlu/ifr-modules)

## FRAME

整套代码使用Plan模式驱动, 每一个功能称为Task, Task之间使用umt工具通讯.  
所有Task组成一个Plan, 可以在[控制台](https://github.com/MineYuanlu/ifr-opencv-web)选择使用哪个Plan.

每个Task具有描述和数据两部分, 描述由代码决定, 用于展示Task的信息。 数据由用户决定, 指示每个Task的属性, 如: 
是否启用此Task、Task的接口通讯名、Task的配置参数等。

## TODO

更新设备后换C++ 20
