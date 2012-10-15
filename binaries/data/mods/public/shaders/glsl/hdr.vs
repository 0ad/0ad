#version 110

varying vec2 v_tex;

void main()
{  
  gl_Position = gl_Vertex;  

  v_tex = vec2(gl_MultiTexCoord0);
}
