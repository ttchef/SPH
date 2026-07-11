
#include <float.h>
#include <raylib.h>

#include <stdlib.h>
#include <stdio.h>

#include <cmath.h>

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

#define MAX_PARTICLES 1000
#define PARTICLE_DISTANCE 30.0f

#define GRAVITY 9.81
#define PIXELS_PER_METER 50.0f
#define SIZE_AMPLIFIER 3.0f
#define SMOOTHING_RADIUS 65.0f

#define TARGET_DENSITY 0.00112
// TODO: Expirement with this
#define VISCOSITY_COEFF 5000.0f

#define FIXED_DT (1.0f / 240.0f)

#define MAX_NEIGHBORS 256

struct entry
{
	unsigned int particle_index;
	unsigned int cell_key;	
};

static inline struct entry entrymake(unsigned int particle_index, unsigned int cell_key)
{
	return (struct entry){particle_index, cell_key};
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

struct simulation
{
	struct entry *spatial_lookup;
	unsigned int *start_indices;
	struct particle *particles;

	// NOTE: interactions settings
	float radius;
	float strength;
};

static inline void simulation_check(struct simulation *s)
{
	assert(s);
	assert(s->spatial_lookup);
	assert(s->start_indices);
	assert(s->particles);
}

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

static inline struct v2u position_to_cell(struct v2 pos, float radius)
{
	unsigned int cell_x = (unsigned int)(pos.x / radius);
	unsigned int cell_y = (unsigned int)(pos.y / radius);
	return v2umake(cell_x, cell_y);	
}

static inline unsigned int cell_hash(struct v2u cell)
{
	unsigned int a = cell.x * 15823;
	unsigned int b = cell.y * 9737333;
	return (a + b) % MAX_PARTICLES;
}

static int entry_compare(const void *a, const void *b)
{
	struct entry e0 = *(struct entry *)a;
	struct entry e1 = *(struct entry *)b;

	return (int)e0.cell_key - (int)e1.cell_key;
}

static void spatial_lookup_update(struct simulation *s)
{
	simulation_check(s);

	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		struct particle *p = &s->particles[i];
		assert(p);

		if (!p->valid)
		{
			continue;
		}

		struct v2u cell = position_to_cell(p->pos, SMOOTHING_RADIUS);
		unsigned int cell_key = cell_hash(cell);
		s->spatial_lookup[i] = entrymake(i, cell_key);
		s->start_indices[i] = UINT32_MAX;
	}

	qsort(s->spatial_lookup, MAX_PARTICLES, sizeof(struct entry), entry_compare);

	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		unsigned int key = s->spatial_lookup[i].cell_key;
		unsigned int key_prev = (i == 0) ? UINT32_MAX : s->spatial_lookup[i - 1].cell_key;

		if (key != key_prev)
		{
			s->start_indices[key] = i;
		}
	}
}

static void simulation_init(struct simulation *s)
{
	assert(s);

	s->particles = calloc(MAX_PARTICLES, sizeof(struct particle));
	particles_init(s->particles);

	s->spatial_lookup = calloc(MAX_PARTICLES, sizeof(struct entry));
	s->start_indices = calloc(MAX_PARTICLES, sizeof(unsigned int));

	s->radius = SMOOTHING_RADIUS * 2;
	s->strength = 2000.0f;

	spatial_lookup_update(s);
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

static int neighbors_get(struct simulation *s, struct v2 pos, float radius, int *out_neighbors)
{
	simulation_check(s);
	assert(out_neighbors);

	int count = 0;
	struct v2u center_cell = position_to_cell(pos, radius);

	for (int dy = -1; dy <= 1; dy++)
	{
		for (int dx = -1; dx <= 1; dx++)
		{
			int cx = (int)center_cell.x + dx;
			int cy = (int)center_cell.y + dy;

			if (cx < 0 || cy < 0)
			{
				continue;
			}

			struct v2u cell = v2umake((unsigned int)cx, (unsigned int)cy);
			unsigned int key = cell_hash(cell);

			unsigned int start = s->start_indices[key];
			if (start == UINT32_MAX)
			{
				continue;
			}

			for (int i = start; i < MAX_PARTICLES; i++)
			{
				if (s->spatial_lookup[i].cell_key != key)
				{
					break;	
				}

				if (count < MAX_NEIGHBORS)
				{
					out_neighbors[count++] = s->spatial_lookup[i].particle_index;
				}
			}
		}
	}

	return count;
}

static float particle_calc_density(struct simulation *s, int particle_index)
{
	simulation_check(s);
	assert(particle_index >= 0 && particle_index < MAX_PARTICLES);

	struct particle *p_i = &s->particles[particle_index];
	assert(p_i);

	if (!p_i->valid)
	{
		return 0.0f;
	}
	
	float density = 0.0f;

	int neighbors[MAX_NEIGHBORS];
	int count = neighbors_get(s, p_i->pos, SMOOTHING_RADIUS, neighbors);

	for (int n = 0; n < count; n++)
	{
		struct particle *p_j = &s->particles[neighbors[n]];
		assert(p_j);

		if (!p_j->valid)
		{
			continue;
		}

		density += p_j->mass * W_poly6_2d(p_i->pos, p_j->pos, SMOOTHING_RADIUS);
	}

	return density;
}

static void particles_update_densities(struct simulation *s)
{
	simulation_check(s);

	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		struct particle *p = &s->particles[i];
		assert(p);

		if (!p->valid)
		{
			continue;
		}

		p->density = particle_calc_density(s, i);
	}
}

// NOTE: Tait equation
static inline float particle_calc_pressure(struct particle *p)
{
	assert(p);

	const float k = 500.0f;

	float term1 = p->density / TARGET_DENSITY;
	term1 = term1 * term1 * term1 * term1 * term1 * term1 * term1;

	float pressure = k * (term1 - 1);

	return pressure < 0.0f ? 0.0f : pressure;
}

static struct v2 particle_calc_pressure_gradient(struct simulation *s, int particle_index)
{
	simulation_check(s);
	assert(particle_index >= 0 && particle_index < MAX_PARTICLES);

	struct particle *p_i = &s->particles[particle_index];
	assert(p_i);

	if (!p_i->valid)
	{
		return v2make(0, 0); 
	}

	struct v2 gradient = v2make(0, 0);
	
	int neighbors[MAX_NEIGHBORS];
	int count = neighbors_get(s, p_i->pos, SMOOTHING_RADIUS, neighbors);

	for (int n = 0; n < count; n++)
	{
		if (particle_index == neighbors[n])
		{
			continue;
		}

		struct particle *p_j = &s->particles[neighbors[n]];
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

static struct v2 particle_calc_velocity_laplacian(struct simulation *s, int particle_index)
{
	simulation_check(s);
	assert(particle_index >= 0 && particle_index < MAX_PARTICLES);

	struct particle *p_i = &s->particles[particle_index];
	assert(p_i);

	if (!p_i->valid)
	{
		return v2make(0, 0); 
	}

	struct v2 force = v2make(0, 0);
	
	int neighbors[MAX_NEIGHBORS];
	int count = neighbors_get(s, p_i->pos, SMOOTHING_RADIUS, neighbors);

	for (int n = 0; n < count; n++)
	{
		if (particle_index == neighbors[n])
		{
			continue;
		}

		struct particle *p_j = &s->particles[neighbors[n]];
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

static struct v2 interaction_force(struct simulation *s, struct v2 input_pos, float radius, float stength, int particle_index)
{
	simulation_check(s);
	
	struct v2 force = v2make(0, 0);
	struct v2 delta = v2sub(input_pos, s->particles[particle_index].pos);
	
	float dist_squared = v2dot(delta, delta);
	if (dist_squared < radius * radius)
	{
		float dist = sqrtf(dist_squared);
		struct v2 dir_to_input_point = (dist <= FLT_EPSILON) ? v2make(0, 0) : v2scale(delta, 1 / dist);

		float center_t = 1 - (dist / radius);

		force = v2sub(v2scale(dir_to_input_point, stength), v2scale(s->particles[particle_index].vel, center_t));
	}

	return force;
}

static void simulation_update(struct simulation *s, float dt)
{
	simulation_check(s);

	spatial_lookup_update(s);
	particles_update_densities(s);

	bool lmb_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
	bool rmb_down = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
	float strength = lmb_down ? s->strength : -s->strength;
	struct v2 mouse_pos = v2fromraylib(GetMousePosition());

	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		struct particle *p = &s->particles[i];
		assert(p);

		if (!p->valid)
		{
			continue;
		}

		struct v2 acc = v2make(0, GRAVITY * PIXELS_PER_METER);
		acc = v2add(acc, particle_calc_pressure_gradient(s, i));
		acc = v2add(acc, particle_calc_velocity_laplacian(s, i));

		if (lmb_down || rmb_down)
		{
			acc = v2add(acc, interaction_force(s, mouse_pos, s->radius, strength, i));
		}

		p->vel = v2add(p->vel, v2scale(acc, dt * 0.5f));
		p->pos = v2add(p->pos, v2scale(p->vel, dt));
	}

	spatial_lookup_update(s);
	particles_update_densities(s);

	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		struct particle *p = &s->particles[i];
		assert(p);

		if (!p->valid)
		{
			continue;
		}

		struct v2 acc = v2make(0, GRAVITY * PIXELS_PER_METER);
		acc = v2add(acc, particle_calc_pressure_gradient(s, i));
		acc = v2add(acc, particle_calc_velocity_laplacian(s, i));

		if (lmb_down || rmb_down)
		{
			acc = v2add(acc, interaction_force(s, mouse_pos, s->radius, strength, i));
		}
		
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

static void simulation_deint(struct simulation *s)
{
	simulation_check(s);
	free(s->spatial_lookup);
	free(s->start_indices);
	free(s->particles);
}

int main(void)
{
	SetConfigFlags(FLAG_MSAA_4X_HINT);
	SetTraceLogLevel(LOG_WARNING);
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "SPH");

	struct simulation simulation = {0};
	simulation_init(&simulation);
	
	float density = particle_calc_density(&simulation, MAX_PARTICLES / 2);
	printf("Target Density: %.5f\n", density);

	float time = 0.0f;
	while (!WindowShouldClose())
	{
		float dt = GetFrameTime();
		time += dt;

		int updates_this_frame = 0;
		while (time >= FIXED_DT)
		{
			simulation_update(&simulation, FIXED_DT);
			time -= FIXED_DT;

			updates_this_frame++;
			if (updates_this_frame >= 5)
			{
				time = 0;
				break;
			}
		}

		const float max_strength = 10000.0f;
		if (IsKeyDown(KEY_LEFT_SHIFT))
		{
			float delta = GetMouseWheelMove();
			simulation.radius += delta * 15.0f;
		}
		if (IsKeyDown(KEY_LEFT_CONTROL))
		{
			float delta = GetMouseWheelMove();
			simulation.strength += delta * 100.0f;

			if (simulation.strength < 0.0f)
			{
				simulation.strength = 0.0f;
			}
			else if (simulation.strength > max_strength)
			{
				simulation.strength = max_strength;
			}
		}

		bool draw_radius = IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT);

		BeginDrawing();
		ClearBackground(BLACK);

		particles_draw(simulation.particles);

		if (draw_radius)
		{
			Vector2 mouse_pos = GetMousePosition();

			float t = simulation.strength / max_strength;
			Color c = (Color){
				.a = 255,
				.r = (unsigned char)(255.0f * t),
				.g = (unsigned char)(255.0f * (1.0f - t)),
				.b = 0,
			};
			DrawCircleLinesV(mouse_pos, simulation.radius, c);
		}

		DrawFPS(10, 10);
		
		EndDrawing();
	}

	simulation_deint(&simulation);
	CloseWindow();
	return 0;
}
