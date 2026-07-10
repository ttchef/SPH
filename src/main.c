
#include <raylib.h>

// NOTE: dont wanna use this cooked thingy
// just so i can have a good conversion func (see v2fromraylib function)
#include <raymath.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

#define MAX_PARTICLES 1000
#define PARTICLE_DISTANCE 30.0f

#define GRAVITY 9.81
#define PIXELS_PER_METER 50.0f
#define SIZE_AMPLIFIER 6.0f
#define SMOOTHING_RADIUS 65.0f

#define TARGET_DENSITY 0.00112
// TODO: Expirement with this
#define VISCOSITY_COEFF 5000.0f

#define FIXED_DT (1.0f / 60.0f) 

//
// NOTE: Math
// 

struct v2
{
	float x;
	float y;
};

static inline struct v2 v2make(float x, float y)
{
	return (struct v2){x, y};
}

static inline struct v2 v2fromraylib(Vector2 raylib_vec)
{
	return v2make(raylib_vec.x, raylib_vec.y);
}

static inline Vector2 v2toraylib(struct v2 v)
{
	return (Vector2){v.x, v.y};
}

static inline struct v2 v2negate(struct v2 v)
{
	return v2make(-v.x, -v.y);
}

static inline float v2lensquared(struct v2 v)
{
	return v.x * v.x + v.y * v.y;	
}

static inline float v2len(struct v2 v)
{
	return sqrtf(v2lensquared(v));
}

static inline struct v2 v2sub(struct v2 a, struct v2 b)
{
	return v2make(a.x - b.x, a.y - b.y);	
}

static inline struct v2 v2add(struct v2 a, struct v2 b)
{
	return v2make(a.x + b.x, a.y + b.y);
}

static inline struct v2 v2scale(struct v2 v, float s)
{
	return v2make(v.x * s, v.y * s);
}

static inline float v2dist(struct v2 a, struct v2 b)
{
	return v2len(v2sub(a, b));
}

static inline float v2dot(struct v2 a, struct v2 b)
{
	return a.x * b.x + a.y * b.y;
}

struct particle
{
	struct v2 pos;
	struct v2 vel;
	float mass;

	// NOTE: acts as a cache and gets updated every frame
	float density;

	bool valid;
};

static void particles_init(struct particle *particles)
{
	assert(particles);

	struct v2 start = v2make(WINDOW_WIDTH / 2 - PARTICLE_DISTANCE * sqrtl(MAX_PARTICLES) * 0.5f, WINDOW_HEIGHT / 2 - PARTICLE_DISTANCE * sqrtl(MAX_PARTICLES) * 0.5f);

	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		struct particle *p = &particles[i];
		assert(p);

		// NOTE: grid positioning
		*p = (struct particle){
			.valid = true,
			.mass = 1.0f,
			.pos = v2make((i % (int)sqrtl(MAX_PARTICLES)) * PARTICLE_DISTANCE + start.x, (i / (int)sqrtl(MAX_PARTICLES)) * PARTICLE_DISTANCE + start.y),
		};

		// NOTE: random positioning
		/*
		*p = (struct particle){
			.valid = true,
			.pos.x = rand() % WINDOW_WIDTH,
			.pos.y = rand() % WINDOW_HEIGHT,
			.mass = 1,
		};
		*/
	}
}

//
// NOTE: Smoothing Kernels
//

// NOTE: Used for density calculation of the particles 
static float W_poly6_2d(struct v2 p_i, struct v2 p_j, float r)
{
	float d = v2dist(p_i, p_j);
	
	if (d < 0 || d > r)
	{
		return 0.0f;
	}

	float normalization = 4 / (PI * r * r * r * r * r * r * r * r);
	float shape_func = (r * r) - (d * d);
	shape_func = shape_func * shape_func * shape_func;

	return normalization * shape_func;
}

static struct v2 W_poly6_2d_gradient(struct v2 p_i, struct v2 p_j, float r)
{
	const float step_size = 0.001f;

	float dx = W_poly6_2d(v2make(p_i.x + step_size, p_i.y), p_j, r) - W_poly6_2d(p_i, p_j, r);	
	float dy = W_poly6_2d(v2make(p_i.x, p_i.y + step_size), p_j, r) - W_poly6_2d(p_i, p_j, r);

	struct v2 gradient = v2scale(v2make(dx, dy), 1 / step_size);

	return gradient;
}

static float W_spiky_2d(struct v2 p_i, struct v2 p_j, float r)
{
	float d = v2dist(p_i, p_j);

	if (d < 0 || d > r)
	{
		return 0.0f;
	}

	float normalization = 3 / (2 * PI * r * r * r * r);
	float shape_func = r - d;
	shape_func = shape_func * shape_func;

	return normalization * shape_func;
}

static struct v2 W_spiky_2d_gradient(struct v2 p_i, struct v2 p_j, float r)
{
	float d = v2dist(p_i, p_j);

	if (d <= 0.0001f || d > r)
	{
		return v2make(0, 0);
	}

	float scale = -3.0f / (PI * r * r * r * r) * (r - d) / d;

	return v2scale(v2sub(p_i, p_j), scale);
}

static float W_viscosity_2d(struct v2 p_i, struct v2 p_j, float r)
{
	float d = v2dist(p_i, p_j);

	if (d < 0 || d > r)
	{
		return 0.0f;
	}

	float normalization = (3 * PI * r * r) / 10;
	float shape_func = -((d * d * d) / (2 * r * r * r)) + ((d * d) / (r * r)) + (r / (2 * d)) - 1;
	shape_func = shape_func * shape_func;

	return normalization * shape_func;
}

static struct v2 W_viscosity_2d_gradient(struct v2 p_i, struct v2 p_j, float r)
{
	const float step_size = 0.001f;

	float dx = W_viscosity_2d(v2make(p_i.x + step_size, p_i.y), p_j, r) - W_viscosity_2d(p_i, p_j, r);	
	float dy = W_viscosity_2d(v2make(p_i.x, p_i.y + step_size), p_j, r) - W_viscosity_2d(p_i, p_j, r);

	struct v2 gradient = v2scale(v2make(dx, dy), 1 / step_size);

	return gradient;
}


// NOTE: maybe not needed?
//       special property no need to calculate from normal functions
static float W_viscosity_laplacian(struct v2 p_i, struct v2 p_j, float r)
{
    float d = v2dist(p_i, p_j);
    if (d <= 0.0001f || d >= r) return 0.0f;
    
    return (45.0f / (PI * r * r * r * r * r * r)) * (r - d);
}

static float particle_calc_density(struct particle *particles, int particle_index)
{
	assert(particles);
	assert(particle_index >= 0 && particle_index < MAX_PARTICLES);

	struct particle *p_i = &particles[particle_index];
	assert(p_i);

	if (!p_i->valid)
	{
		return 0.0f;
	}
	
	float density = 0.0f;

	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		struct particle *p_j = &particles[i];
		assert(p_j);

		if (!p_j->valid)
		{
			continue;
		}

		density += p_j->mass * W_poly6_2d(p_i->pos, p_j->pos, SMOOTHING_RADIUS);
	}

	return density;
}

static void particles_update_densities(struct particle *particles)
{
	assert(particles);

	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		struct particle *p = &particles[i];
		assert(p);

		if (!p->valid)
		{
			continue;
		}

		p->density = particle_calc_density(particles, i);
	}
}

// NOTE: Tait equation
static inline float particle_calc_pressure(struct particle *p)
{
	assert(p);

	const float k = 500.0f;

	float term1 = p->density / TARGET_DENSITY;
	term1 = term1 * term1 * term1 * term1 * term1 * term1 * term1;

	float result = k * (term1 - 1);

	return result < 0.0f ? 0.0f : result;
}

/*
// NOTE: normalized linear Equation of State (EOS)
static inline float particle_calc_pressure(struct particle *p)
{
	assert(p);

	const float k = 500.0f;
	float result = k * ((p->density / TARGET_DENSITY) - 1.0f);

	return result < 0.0f ? 0.0f : result;
}
*/

static struct v2 particle_calc_pressure_gradient(struct particle *particles, int particle_index)
{
	assert(particles);
	assert(particle_index >= 0 && particle_index < MAX_PARTICLES);

	struct particle *p_i = &particles[particle_index];
	assert(p_i);

	if (!p_i->valid)
	{
		return v2make(0, 0); 
	}

	struct v2 gradient = v2make(0, 0);

	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		if (particle_index == i)
		{
			continue;
		}

		struct particle *p_j = &particles[i];
		assert(p_j);

		if (!p_j->valid)
		{
			continue;
		}

		struct v2 smoothing_gradient = W_spiky_2d_gradient(p_i->pos, p_j->pos, SMOOTHING_RADIUS);

		float term1 = particle_calc_pressure(p_i) / (p_i->density * p_i->density);
		float term2 = particle_calc_pressure(p_j) / (p_j->density * p_j->density);

		smoothing_gradient = v2scale(smoothing_gradient, p_j->mass * (term1 + term2));
		gradient = v2add(gradient, smoothing_gradient);
	}

	return v2negate(gradient);
}

static struct v2 particle_calc_velocity_laplacian(struct particle *particles, int particle_index)
{	
	assert(particles);
	assert(particle_index >= 0 && particle_index < MAX_PARTICLES);

	struct particle *p_i = &particles[particle_index];
	assert(p_i);

	if (!p_i->valid)
	{
		return v2make(0, 0); 
	}

	struct v2 force = v2make(0, 0);

	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		if (particle_index == i)
		{
			continue;
		}

		struct particle *p_j = &particles[i];
		assert(p_j);

		if (!p_j->valid)
		{
			continue;
		}

		struct v2 v_ij = v2sub(p_i->vel, p_j->vel);
		struct v2 x_ij = v2sub(p_i->pos, p_j->pos);

		struct v2 smoothing = W_spiky_2d_gradient(p_i->pos, p_j->pos, SMOOTHING_RADIUS);
		// printf("POS: (%.4f, %.4f) (%.4f, %.4f)\n", p_i->pos.x, p_i->pos.y, p_j->pos.x, p_j->pos.y);
		// printf("SMOOTHING: %.4f, %.4f\n", smoothing.x, smoothing.y);
		// exit(1);

		float numerator = v2dot(x_ij, smoothing);
		float denominator = v2dot(x_ij, x_ij) + 0.01 * SMOOTHING_RADIUS * SMOOTHING_RADIUS;

		struct v2 result = v2scale(v_ij, (p_j->mass / p_j->density) * (numerator / denominator) * VISCOSITY_COEFF);
		force = v2add(force, result);

		/*
		struct v2 dv = v2sub(p_j->vel, p_i->vel);
		float laplacian = W_viscosity_laplacian(p_i->pos, p_j->pos, SMOOTHING_RADIUS);
		struct v2 viscosity_force = v2scale(dv, (p_j->mass / p_j->density) * VISCOSITY_COEFF * laplacian);

		force = v2add(force, viscosity_force);
		*/
	}

	force = v2scale(force, 2.0f);
	return force;
}

static void particles_update(struct particle *particles, float dt)
{
	assert(particles);

	particles_update_densities(particles);

	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		struct particle *p = &particles[i];
		assert(p);

		if (!p->valid)
		{
			continue;
		}

		struct v2 acc = v2make(0, GRAVITY * PIXELS_PER_METER);
		acc = v2add(acc, particle_calc_pressure_gradient(particles, i));
		acc = v2add(acc, particle_calc_velocity_laplacian(particles, i));

		p->vel = v2add(p->vel, v2scale(acc, dt * 0.5f));
		p->pos = v2add(p->pos, v2scale(p->vel, dt));
	}

	particles_update_densities(particles);

	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		struct particle *p = &particles[i];
		assert(p);

		if (!p->valid)
		{
			continue;
		}

		struct v2 acc = v2make(0, GRAVITY * PIXELS_PER_METER);
		acc = v2add(acc, particle_calc_pressure_gradient(particles, i));
		acc = v2add(acc, particle_calc_velocity_laplacian(particles, i));
		
		p->vel = v2add(p->vel, v2scale(acc, dt * 0.5f));

		// NOTE: collisions
		const float radius = p->mass + SIZE_AMPLIFIER;
		const float damping = 0.67f;

		if (p->pos.x - radius < 0)
		{
			p->pos.x = radius;
			p->vel.x = -p->vel.x * damping;
		}
		else if (p->pos.x + radius > WINDOW_WIDTH)
		{
			p->pos.x = WINDOW_WIDTH - radius;
			p->vel.x = -p->vel.x * damping;
		}

		if (p->pos.y - radius < 0)
		{
			p->pos.y = radius;
			p->vel.y = -p->vel.y * damping;
		}
		else if (p->pos.y + radius > WINDOW_HEIGHT)
		{
			p->pos.y = WINDOW_HEIGHT - radius;
			p->vel.y = -p->vel.y * damping;
		}
	}
}

static void particles_draw(struct particle *particles)
{
	assert(particles);

	// NOTE: last color for speed
	const float max_speed = 800.0f;

	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		struct particle *p = &particles[i];
		assert(p);

		if (!p->valid)
		{
			continue;
		}

		float radius = p->mass + SIZE_AMPLIFIER;

		float speed = v2len(p->vel);
		float t = speed / max_speed;

		if (t > 1.0f)
		{
			t = 1.0f;
		}

		Color c = (Color){
			.r = (unsigned char)(255.0f * t),
			.g = 0,
			.b = (unsigned char)(255.0f * (1.0f - t)),
			.a = 255,
		};
		DrawCircleV(v2toraylib(p->pos), radius, c);
	}
}

int main(void)
{
	SetConfigFlags(FLAG_MSAA_4X_HINT);
	SetTraceLogLevel(LOG_WARNING);
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "SPH");

	struct particle *particles = calloc(MAX_PARTICLES, sizeof(struct particle));
	particles_init(particles);

	float density = particle_calc_density(particles, MAX_PARTICLES / 2);
	printf("Target Density: %.5f\n", density);

	float time = 0.0f;
	while (!WindowShouldClose())
	{
		float dt = GetFrameTime();
		time += dt;

		int updates_this_frame = 0;
		while (time >= FIXED_DT)
		{
			particles_update(particles, FIXED_DT);
			time -= FIXED_DT;

			updates_this_frame++;
			if (updates_this_frame >= 5)
			{
				time = 0;
				break;
			}
		}

		BeginDrawing();
		ClearBackground(BLACK);

		particles_draw(particles);

		DrawFPS(10, 10);
		
		EndDrawing();
	}

	CloseWindow();
	return 0;
}
