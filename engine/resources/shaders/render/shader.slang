struct VertexInput {
  float3 position : POSITION;
  float3 color: COLOR;
};

struct VertexOutput {
  float4 position : SV_Position;
  float3 color: COLOR;
};

[shader("vertex")]
VertexOutput vertMain(VertexInput input) {
  VertexOutput output;

  output.position = float4(input.position, 1.0);
  output.color = float3(1.0, 1.0, 1.0);

  return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target {
  float4 color = float4(input.color, 1.0);

  return color;
}
