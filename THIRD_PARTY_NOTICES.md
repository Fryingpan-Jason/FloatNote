# Third-party notices

## Liquid DOM

FloatNote 2.0 adapts the quartic/lip height profile, profile derivative,
and layered rim/background-reflection ideas from AndrewPrifer/liquid-dom:

https://github.com/AndrewPrifer/liquid-dom/blob/dd342ab663d718e8b4edf4cd39983b9a873360a7/packages/core/src/shaders.ts

The code is ported to C++/HLSL for a single native window. The circular profile,
bounded slope handling, sampling filter, integration and experiment controls differ
from the upstream renderer. No DOM, WebGPU, layout or content-atlas code is included.

MIT License

Copyright (c) 2026 Andras Prifer

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
