cbuffer Camera {
    column_major matrix world;
    column_major matrix view;
    column_major matrix proj;
    column_major matrix mvp;
}

float4 apply_camera(const float4 position) {    
    return mul(mvp, position);
}
