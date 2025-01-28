struct UniformBufferObject {
  float4x4 model;
  float4x4 view;
  float4x4 projection;
};

struct PushConstants {
  float3 offset;
  float3 color;
};

[vk::binding(0)]
ConstantBuffer<UniformBufferObject> ubo;

[vk::binding(1)]
Sampler2D tex;

struct VertexInput {
  float3 position : POSITION;
  float3 color : COLOR;
  float2 uv : TEXCOORD;
};

struct VertexOutput {
  float4 position : SV_Position;
  float3 color : COLOR;
  float2 uv : TEXCOORD;
};

[shader("vertex")]
VertexOutput vertMain(VertexInput input, uniform PushConstants push) {
  VertexOutput output;

  float4x4 modelViewProjection = mul(ubo.model, mul(ubo.view, ubo.projection));
  output.position = mul(transpose(modelViewProjection), float4(input.position, 1.0));
  // output.position = float4(input.position + push.offset, 1.0);
  output.color = input.color;
  output.uv = input.uv;

  return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input, uniform PushConstants push) : SV_Target {
  float4 color = float4(tex.Sample(input.uv).xyz, 1.0);

  return color;
}
