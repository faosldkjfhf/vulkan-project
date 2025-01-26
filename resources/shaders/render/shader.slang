struct UniformBufferObject {
  float4x4 model;
  float4x4 view;
  float4x4 projection;
};

[vk::binding(0)]
ConstantBuffer<UniformBufferObject> ubo;

struct VertexInput {
  float3 position : POSITION;
  float3 color : COLOR;
};

struct VertexOutput {
  float4 position : SV_Position;
  float3 color : COLOR;  
};

[shader("vertex")]
VertexOutput vertMain(VertexInput input) {
  VertexOutput output;

  float4x4 modelViewProjection = mul(ubo.model, mul(ubo.view, ubo.projection));
  output.position = mul(transpose(modelViewProjection), float4(input.position, 1.0));
  output.color = input.color;

  return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target {
  float4 color = float4(input.color, 1.0);

  return color;
}
