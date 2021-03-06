#include "scene.h"
#include "intersect.h"
#include "montecarlo.h"

#include <thread>
using std::thread;

int clamp_tile(int n, int val, bool t) {
	if (t == true)
	{
		n = n % val;
		if (n < 0) 
			n += val;
	}
	else
		n = clamp(n, 0, val - 1);

	return n;
}


// lookup texture value
vec3f lookup_scaled_texture(vec3f value, image3f* texture, vec2f uv, bool tile = false) {
    // YOUR CODE GOES HERE ----------------------
	if (texture != nullptr)
	{
		int i = int(uv.x * texture->width());
		int j = int(uv.y * texture->height());

		float s = (uv.x * texture->width()) - i;
		float t = (uv.y * texture->height()) - j;

		int ii = i + 1;
		int jj = j + 1;

		i = clamp_tile(i, texture->width(), tile);
		j = clamp_tile(j, texture->height(), tile);
		ii = clamp_tile(ii, texture->width(), tile);
		jj = clamp_tile(jj, texture->height(), tile);

		value *= texture->at(i, j)*(1 - s)*(1 - t) +
			texture->at(i, jj)*(1 - s)*t +
			texture->at(ii, j)*s*(1 - t) +
			texture->at(ii, jj)*s*t;

		return value;
	}
    return value; // placeholder
}

// compute the brdf
vec3f eval_brdf(vec3f kd, vec3f ks, float n, vec3f v, vec3f l, vec3f norm, bool microfacet) {
    // YOUR CODE GOES HERE ----------------------
    auto h = normalize(v+l); // placeholder (non-microfacet model)
	vec3f resulting_brdf = zero3f;

	if (!microfacet)
	{
		resulting_brdf = kd / pif + ks*(n + 8) / (8 * pif) * pow(max(0.0f, dot(norm, h)), n); // placeholder (non-microfacet model)
		return resulting_brdf;
	}
	else
	{
		auto d = ((n + 2) / (2 * pif)) * pow(max(0.0f, dot(h, norm)), n);
		auto f = ks + (one3f - ks) * (1 - dot(h, l));
		auto g_temp = min((2.0f * dot(h, norm) * dot(v, norm)) / dot(v, h), 
			(2.0f * dot(h, norm) * dot(l, norm)) / dot(l, h));
		auto g = min(1.0f, g_temp);

		resulting_brdf = (d * g * f) / (4.0f * dot(l, norm) * dot(v, norm));
		return resulting_brdf;
	}
}

// evaluate the environment map
vec3f eval_env(vec3f ke, image3f* ke_txt, vec3f dir) {

    // YOUR CODE GOES HERE ----------------------
	float u = atan2(dir.x, dir.z) / (2.0f * pif);
	float v = 1.0f - acos(dir.y) / pif;
	vec2f uv = vec2f(u, v);
	auto lookup_value = lookup_scaled_texture(ke, ke_txt, uv, true);

    if(! ke_txt) return ke;
    else return lookup_value;
}

// pick a direction according to the cosine (returns direction and its pdf)
pair<vec3f,float> sample_cosine(vec3f norm, vec2f ruv) {
    auto frame = frame_from_z(norm);
    auto l_local = sample_direction_hemispherical_cosine(ruv);
    auto pdf = sample_direction_hemispherical_cosine_pdf(l_local);
    auto l = transform_direction(frame, l_local);
    return {l,pdf};
}

// pick a direction according to the brdf (returns direction and its pdf)
pair<vec3f,float> sample_brdf(vec3f kd, vec3f ks, float n, vec3f v, vec3f norm, vec2f ruv, float rl) {
    if(ks == zero3f) return sample_cosine(norm, ruv);
    auto frame = frame_from_z(norm);
    auto dw = mean(kd) / (mean(kd) + mean(ks));
    auto v_local = transform_direction_inverse(frame, v);
    auto l_local = zero3f, h_local = zero3f;
    if(rl < dw) {
        l_local = sample_direction_hemispherical_cosine(ruv);
        h_local = normalize(l_local+v_local);
    } else {
        h_local = sample_direction_hemispherical_cospower(ruv, n);
        l_local = -v_local + h_local*2*dot(v_local,h_local);
    }
    auto l = transform_direction(frame, l_local);
    auto dpdf = sample_direction_hemispherical_cosine_pdf(l_local);
    auto spdf = sample_direction_hemispherical_cospower_pdf(h_local,n) / (4*dot(v_local,h_local));
    auto pdf = dw * dpdf + (1-dw) * spdf;
    return {l,pdf};
}

// compute the color corresponing to a ray by pathtrace
vec3f pathtrace_ray(Scene* scene, ray3f ray, Rng* rng, int depth) {
    // get scene intersection
    auto intersection = intersect(scene,ray);
    
    // if not hit, return background (looking up the texture by converting the ray direction to latlong around y)
    if(! intersection.hit) {

        // YOUR CODE GOES HERE ----------------------
		return eval_env(scene->background, scene->background_txt, ray.d);

    }
    
    // setup variables for shorter code
    auto pos = intersection.pos;
    auto norm = intersection.norm;
    auto v = -ray.d;
	auto t_coord = intersection.texcoord;

    // compute material values by looking up textures
    // YOUR CODE GOES HERE ----------------------
	auto ke = lookup_scaled_texture(intersection.mat->ke, intersection.mat->ke_txt, t_coord, true);
	auto kd = lookup_scaled_texture(intersection.mat->kd, intersection.mat->kd_txt, t_coord, true);
	auto ks = lookup_scaled_texture(intersection.mat->ks, intersection.mat->ks_txt, t_coord, true);

	norm = lookup_scaled_texture(norm, intersection.mat->norm_txt, t_coord, true);

    auto n = intersection.mat->n;
    auto mf = intersection.mat->microfacet;
    
    // accumulate color starting with ambient
    auto c = scene->ambient * kd;
    
    // add emission if on the first bounce
    // YOUR CODE GOES HERE ----------------------
	if (depth == 0)
		c += ke;
    
    // foreach point light
    for(auto light : scene->lights) {
        // compute light response
        auto cl = light->intensity / (lengthSqr(light->frame.o - pos));
        // compute light direction
        auto l = normalize(light->frame.o - pos);
        // compute the material response (brdf*cos)
        auto brdfcos = max(dot(norm,l),0.0f) * eval_brdf(kd, ks, n, v, l, norm, mf);
        // multiply brdf and light
        auto shade = cl * brdfcos;
        // check for shadows and accumulate if needed
        if(shade == zero3f) continue;
        // if shadows are enabled
        if(scene->path_shadows) {
            // perform a shadow check and accumulate
            if(! intersect_shadow(scene,ray3f::make_segment(pos,light->frame.o))) c += shade;
        } else {
            // else just accumulate
            c += shade;
        }
    }
    
    // YOUR AREA LIGHT CODE GOES HERE ----------------------
	// foreach surface
	for (auto surf_ : scene->surfaces)
	{
		// skip if no emission from surface
		if (surf_->mat->ke == zero3f)
			continue;

		// pick a point on the surface, grabbing normal, area and texcoord
		vec2f uv;
		vec3f N_l, S;
		float area;

		// check if quad
		if (surf_->isquad)
		{
			// generate a 2d random number
			uv = rng->next_vec2f();

			// compute light position, normal, area
			S = transform_point(surf_->frame, 2.0f * surf_->radius * vec3f(uv.x - 0.5f, uv.y - 0.5f, 0.0f)); // ????
			N_l = transform_normal(surf_->frame, vec3f(0.0f, 0.0f, 1.0f));
			area = sqr(2.0f * surf_->radius);

			// set tex coords as random value got before
			intersection.texcoord = uv;
		}

		// else
		else
		{
			// generate a 2d random number
			uv = rng->next_vec2f();

			// compute light position, normal, area
			S = transform_point(surf_->frame, 2.0f * surf_->radius * vec3f(uv.x - 0.5, uv.y - 0.5, 0.0f)); // ????
			N_l = transform_normal(surf_->frame, vec3f(0.0f, 0.0f, 1.0f));
			area = pif * sqr(sqr(surf_->radius));

			// set tex coords as random value got before
			intersection.texcoord = uv;
		}

		// get light emission from material and texture
		vec3f ke_alight = lookup_scaled_texture(surf_->mat->ke, surf_->mat->ke_txt, uv);
		vec3f L_e = ke_alight * area;

		// compute light direction
		vec3f light_dir = normalize(S - pos);

		// compute light response
		vec3f c_l = L_e * max(0.0f, -(1) * dot(N_l, light_dir)) / lengthSqr(pos - S);

		// compute the material response (brdf*cos)
		vec3f brdfcos = max(dot(norm, light_dir), 0.0f) * eval_brdf(kd, ks, n, v, light_dir, norm, mf);
		
		// multiply brdf and light
		auto shade = c_l * brdfcos;
		if (shade == zero3f)
			continue;

		// check for shadows and accumulate if needed
		// if shadows are enabled
		if (scene->path_shadows)
		{
			// perform a shadow check and accumulate
			auto shadow_ray = ray3f::make_segment(pos, S);
			if (!intersect_shadow(scene, shadow_ray))
				c += shade;
		}

		// else
		else
			// else just accumulate
			c += shade;
	}
    
    // YOUR ENVIRONMENT LIGHT CODE GOES HERE ----------------------
	if (scene->background_txt != nullptr)
	{
		// sample the brdf for environment illumination if the environment is there
		auto env_sample = sample_brdf(kd, ks, n, v, norm, rng->next_vec2f(), rng->next_float());

		// pick direction and pdf
		vec3f env_dir = env_sample.first;
		float env_pdf = env_sample.second;

		// compute the material response (brdf*cos)
		vec3f brdfcos = max(dot(norm, env_dir), 0.0f) * eval_brdf(kd, ks, n, v, env_dir, norm, mf);

		// accumulate recersively scaled by brdf*cos/pdf
		vec3f c_env = eval_env(scene->background, scene->background_txt, env_dir) / env_pdf;
		vec3f shade = brdfcos * c_env;

		// if shadows are enabled
		if (scene->path_shadows)
		{
			// perform a shadow check and accumulate
			auto shadow_ray_env = ray3f(pos, env_dir);
			if (!intersect_shadow(scene, shadow_ray_env))
				c += shade;
		}

		// else
		else
			// else just accumulate
			c += shade;
	}      

    // YOUR INDIRECT ILLUMINATION CODE GOES HERE ----------------------
	if (depth < scene->path_max_depth)
	{
		// sample the brdf for indirect illumination
		auto ind_sample = sample_brdf(kd, ks, n, v, norm, rng->next_vec2f(), rng->next_float());

		// pick direction and pdf
		vec3f ind_dir = ind_sample.first;
		float ind_pdf = ind_sample.second;

		// compute the material response (brdf*cos)
		vec3f brdfcos = max(dot(norm, ind_dir), 0.0f) * eval_brdf(kd, ks, n, v, ind_dir, norm, mf);
		ray3f ind_ray = ray3f(pos, ind_dir);

		// accumulate recersively scaled by brdf*cos/pdf
		vec3f c_ind = pathtrace_ray(scene, ind_ray, rng, depth + 1) / ind_pdf; //depth??
		vec3f shade = brdfcos * c_ind;

		c += shade;
	}
    
    // return the accumulated color
    return c;
}

// pathtrace an image
void pathtrace(Scene* scene, image3f* image, RngImage* rngs, int offset_row, int skip_row, bool verbose) {
    if(verbose) message("\n  rendering started        ");
    // foreach pixel
    for(auto j = offset_row; j < scene->image_height; j += skip_row ) {
        if(verbose) message("\r  rendering %03d/%03d        ", j, scene->image_height);
        for(auto i = 0; i < scene->image_width; i ++) {
            // init accumulated color
            image->at(i,j) = zero3f;
            // grab proper random number generator
            auto rng = &rngs->at(i, j);
            // foreach sample
            for(auto jj : range(scene->image_samples)) {
                for(auto ii : range(scene->image_samples)) {
                    // compute ray-camera parameters (u,v) for the pixel and the sample
                    auto u = (i + (ii + rng->next_float())/scene->image_samples) /
                        scene->image_width;
                    auto v = (j + (jj + rng->next_float())/scene->image_samples) /
                        scene->image_height;
                    // compute camera ray
                    auto ray = transform_ray(scene->camera->frame,
                        ray3f(zero3f,normalize(vec3f((u-0.5f)*scene->camera->width,
                                                     (v-0.5f)*scene->camera->height,-1))));

					// ------------ EXTRA - DEPTH OF FIELD -----------//
					if (scene->aperture != 0)
					{
						float a = scene->aperture;
						float focus = scene->f_depth;
						auto pixel = vec2f(scene->camera->width / scene->image_width,
							scene->camera->height / scene->image_height);

						// uniformly sampled 2D points
						vec2f s = rng->next_vec2f();
						vec2f r = rng->next_vec2f();

						// Point on the lens plane
						vec3f F = zero3f + 
							vec3f((0.5 - s.x) * a, (0.5f - s.y) * a, 0.0f);
						
						// Point on the image plane
						vec3f Q = zero3f + 
							vec3f((i + 0.5f - r.x) * pixel.x - 0.5, 
							(j + 0.5 - r.y) * pixel.y - 0.5, 0.0f)*focus - 
							vec3f(0, 0, focus);

						// New aperture ray
						ray3f aperture_ray = ray3f(F, normalize(Q - F));

						ray = transform_ray(scene->camera->frame, aperture_ray);
					}

                    // set pixel to the color raytraced with the ray
                    image->at(i,j) += pathtrace_ray(scene,ray,rng,0);
                }
            }

            // scale by the number of samples
            image->at(i,j) /= (scene->image_samples*scene->image_samples);
        }
    }
    if(verbose) message("\r  rendering done        \n");
    
}

// pathtrace an image with multithreading if necessary
image3f pathtrace(Scene* scene, bool multithread) {
    // allocate an image of the proper size
    auto image = image3f(scene->image_width, scene->image_height);
    
    // create a random number generator for each pixel
    auto rngs = RngImage(scene->image_width, scene->image_height);

    // if multitreaded
    if(multithread) {
        // get pointers
        auto image_ptr = &image;
        auto rngs_ptr = &rngs;
        // allocate threads and pathtrace in blocks
        auto threads = vector<thread>();
        auto nthreads = thread::hardware_concurrency();
        for(auto tid : range(nthreads)) threads.push_back(thread([=](){
            return pathtrace(scene,image_ptr,rngs_ptr,tid,nthreads,tid==0);}));
        for(auto& thread : threads) thread.join();
    } else {
        // pathtrace all rows
        pathtrace(scene, &image, &rngs, 0, 1, true);
    }
    
    // done
    return image;
}

// runs the raytrace over all tests and saves the corresponding images
int main(int argc, char** argv) {
    auto args = parse_cmdline(argc, argv,
        { "05_pathtrace", "raytrace a scene",
            {  {"resolution", "r", "image resolution", "int", true, jsonvalue() }  },
            {  {"scene_filename", "", "scene filename", "string", false, jsonvalue("scene.json")},
               {"image_filename", "", "image filename", "string", true, jsonvalue("")}  }
        });
    auto scene_filename = args.object_element("scene_filename").as_string();
    auto image_filename = (args.object_element("image_filename").as_string() != "") ?
        args.object_element("image_filename").as_string() :
        scene_filename.substr(0,scene_filename.size()-5)+".png";
    auto scene = load_json_scene(scene_filename);
    if(! args.object_element("resolution").is_null()) {
        scene->image_height = args.object_element("resolution").as_int();
        scene->image_width = scene->camera->width * scene->image_height / scene->camera->height;
    }
    accelerate(scene);
	std::chrono::high_resolution_clock::time_point t_0, t_1;
	t_0 = std::chrono::high_resolution_clock::now();
    message("rendering %s ... ", scene_filename.c_str());
    auto image = pathtrace(scene,true);
    write_png(image_filename, image, true);
    delete scene;
    message("done\n");
	t_1 = std::chrono::high_resolution_clock::now();
	double execution_time = (std::chrono::duration_cast<std::chrono::microseconds>(t_1 - t_0).count() / 1e06);
	message("Execution time: %f s \n", execution_time);
}
