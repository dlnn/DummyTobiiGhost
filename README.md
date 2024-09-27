# DummyTobiiGhost
![](https://github.com/dlnn/DummyTobiiGhost/blob/master/Image/1.jpg)

- 逆了~~TobiiGhost~~抄的
- HLSL也是反汇编CSO后自己搓的 已测试效果一样
## 
DX版本 | 是否支持
:-: | :-:
11 | $${\color{lightgreen}√}$$ 
10 | $${\color{lightgreen}√}$$ 
9 | $${\color{yellow}?}$$ ~~感觉可以~~
##
- 小小修改下就能搬迁到IMGUI里
- 搁这修改参数
~~~cpp
	TobiiRenderSettings settings = {};
	//settings.Enable = false;
	//settings.ShapeType = Heatmap;
	settings.Size = 0.8f;
	settings.Color = { 0.0f, 0.0f, 0.0f, 0.8f };
	settings.BackgroundColor = { 1.0f, 1.0f, 1.0f, 0.3f };
~~~
