[vk::image_format("r16f")]
RWTexture2D<float4> image;

struct PushConstants {
  float4 data1;
  float4 data2;
  float4 data3;
  float4 data4;
};

[vk::push_constant()]
ConstantBuffer<PushConstants> constants;

[shader("compute")]
[numthreads(16, 16, 1)]
void computeMain(uint3 threadId: SV_DispatchThreadID, uint3 localThreadId: SV_GroupThreadID) {
  uint2 texelCoord = uint2(threadId.xy);
  uint2 size = { 0, 0 };
  image.GetDimensions(size.x, size.y);

  float4 topColor = constants.data1;
  float4 bottomColor = constants.data2;

  if (texelCoord.x < size.x && texelCoord.y < size.y) {
    /*
    float4 color = float4(0.0, 0.0, 0.0, 1.0);
    if (localThreadId.x != 0 && localThreadId.y != 0) {
      color.x = float(texelCoord.x) / size.x;
      color.y = float(texelCoord.y) / size.y;
    }
    */

    float blend = float(texelCoord.y) / size.y;

    image.Store(texelCoord, lerp(topColor, bottomColor, blend));
  }
}
