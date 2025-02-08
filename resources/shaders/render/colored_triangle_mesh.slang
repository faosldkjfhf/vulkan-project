struct Vertex {
  float3 position;
  float uv_x;
  float3 normal;
  float uv_y;
  float4 color;
};

[vk::push_constant]
cbuffer Constants {
  float4x4 renderMatrix;
  Vertex *vertices;
};

[vk::binding(0, 0)]
Sampler2D texture;

struct VertexOutput {
  float4 position : SV_Position;
  float2 uv : TEXCOORD;
  float4 color : COLOR;
};

[shader("vertex")]
VertexOutput vertMain(int vertexIndex: SV_VertexID) {
  Vertex v = vertices[vertexIndex];

  VertexOutput output;

  output.position = mul(transpose(renderMatrix), float4(v.position, 1.0));
  output.color = v.color;
  output.uv = float2(v.uv_x, v.uv_y);

  return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target {
  float4 color = texture.Sample(input.uv);
  return color;
}

