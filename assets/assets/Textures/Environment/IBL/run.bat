..\..\..\..\..\bin\cmftRelease.exe --input .\rooftop_night_2k.hdr  --outputNum 1 --output0params ktx,rgba32f,cubemap --generateMipChain true  --filter radiance --useOpenCL false --edgeFixup warp --excludeBase true --dstFaceSize 128 --output0 radiance3
..\..\..\..\..\bin\cmftRelease.exe --input .\rooftop_night_2k.hdr  --outputNum 1 --output0params ktx,rgba32f,cubemap --generateMipChain true  --filter  irradiance --dstFaceSize 32 --output0 irradiance3