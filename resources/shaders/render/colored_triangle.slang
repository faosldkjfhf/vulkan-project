static float3 positions[3] = { float3(1.0, 1.0, 0.0), float3(-1.0, 1.0, 0.0), float3(0.0, -1.0, 0.0) };
static float3 colors[3] = { float3(1.0, 0.0, 0.0), float3(0.0, 1.0, 0.0), float3(0.0, 0.0, 1.0) };

struct VertexInput {
  int index : SV_VertexID;
};

struct VertexOutput {
  float4 position : SV_Position;
  float4 color : COLOR;
};

[shader("vertex")]
VertexOutput vertMain(VertexInput input) {
  VertexOutput output;

  output.position = float4(positions[input.index], 1.0);
  output.color = float4(colors[input.index], 1.0);

  return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target {
  float4 color = input.color;
  return color;
}
