#ifndef MATERIALH
#define MATERIALH

#include <random>

#include "hitable.h"
#include "ray.h"
#include "texture.h"



class material{
    public:
        virtual bool scatter(const ray &r_in, const hit_record &rec, vec3 &attenuation, ray &acattered) const = 0;
        virtual vec3 emitted(float u, float v, const vec3& p) const {
            return vec3(0,0,0);
        }
    static std::random_device rd;
    static std::mt19937 gen;
    static std::uniform_real_distribution<> dis;
};

std::mt19937 material::gen = std::mt19937(rd());
std::uniform_real_distribution<> material::dis = std::uniform_real_distribution<>(0, 1);

vec3 random_in_unit_sphere() {
    vec3 p;
    do{
        p = vec3(dis(gen), dis(gen), dis(gen)) * 2.0 - vec3(1, 1, 1);
    } while (p.squared_length() >= 1.0);
    return p;
}

bool refract (const vec3& v, const vec3& n, float ni_over_nt, vec3& refracted){
    vec3 uv = v.unit_vector();
    float dt = uv.dot(n);
    float discriminant = 1.0 - ni_over_nt* ni_over_nt*(1-dt*dt);
    if (discriminant > 0){
        refracted = (uv - n*dt)*ni_over_nt - n*sqrt(discriminant);
        return true;
    } 
    else 
        return false;
}

float schlick(float cosine, float ref_idx){
    float r0 = (1-ref_idx) / (1+ref_idx);
    r0 = r0*r0;
    return r0 + (1-r0)*pow((1-cosine),5);
}

class diffuse_light : public material  {
    public:
        diffuse_light(texture *a) : emit(a) {}
        virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const { return false; }
        virtual vec3 emitted(float u, float v, const vec3& p) const { return emit->value(u, v, p); }
        texture *emit;
};

class lambertian : public material {
    public:
        lambertian(texture *a) : albedo(a) {}
        virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const  {
             vec3 target = rec.p + rec.normal + random_in_unit_sphere();
             scattered = ray(rec.p, target-rec.p, r_in.time());
             attenuation = albedo->value(rec.u, rec.v, rec.p);
             return true;
        }
        texture *albedo;
};

class metal : public material{
public:
    metal(const vec3 &a, float f) : albedo(a) { if(f < 1) fuzz = f; else fuzz = 1; }
    virtual bool scatter(const ray &r_in, const hit_record &rec, vec3 &attenuation, ray &scattered) const
    {
        vec3 reflected = reflect(r_in.direction().unit_vector(), rec.normal);
        scattered = ray(rec.p, reflected + random_in_unit_sphere() * fuzz, r_in.time());
        attenuation = albedo;
        return (scattered.direction().dot(rec.normal) > 0);
    }
    vec3 albedo;
    float fuzz;
};

class dielectric : public material{
    public: 
        dielectric(float ri) : ref_idx(ri) {}
        virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const{
            vec3 outward_normal; 
            vec3 reflected = reflect(r_in.direction(), rec.normal);
            float ni_over_nt;
            attenuation = vec3(1.0,1.0,1.0);
            vec3 refracted;
            float reflect_prob;
            float cosine;
            if(r_in.direction().dot(rec.normal) > 0){
                outward_normal = -rec.normal;
                ni_over_nt = ref_idx;
                cosine = ref_idx * r_in.direction().dot(rec.normal)/ r_in.direction().length();
            } else {
                outward_normal = rec.normal;
                ni_over_nt = 1.0 / ref_idx;
                cosine = -r_in.direction().dot(rec.normal) / r_in.direction().length();
            }
            if (refract(r_in.direction(), outward_normal, ni_over_nt, refracted)) {
                reflect_prob = schlick(cosine, ref_idx);
            } else {
               scattered = ray(rec.p, reflected, r_in.time());
               reflect_prob = 1.0;
            }
            if(dis(gen) < reflect_prob){
                scattered = ray(rec.p, reflected, r_in.time());
            } else {
                scattered = ray(rec.p, refracted, r_in.time());
            }

            return true;
        }
        float ref_idx;
};

#endif