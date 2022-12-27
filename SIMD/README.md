我想让便利单行像素时使用SIMD

但是非常可惜的是，M1 Mac对OpenMP、PSTL的支持很差，Xcode Clang也不支持C++17的`std::execution`，于是并没有成功
