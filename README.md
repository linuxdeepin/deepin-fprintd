# Deepin Fprintd

## What?

Fingerprint backend for deepin. It's based on libfprint, function similar to fprintd. 


## Why?

我们使用过 `fprintd`，它提供了基本的指纹录入与验证接口。但它不满足我们的需求，不能为某个手指录入多组数据，它能录入的数据取决与设备，不能一直录入。

而我们立志做一个体验可以与手机相当的指纹功能，所以我们选择重写。当然现在只是处于实验阶段，我们的想法需要测试，预祝我们一切顺利。
