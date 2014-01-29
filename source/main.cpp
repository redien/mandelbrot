
#include <limbus/OpenglWindow.hpp>
#include <limbus/Timer.hpp>
#include <limbus/opengl.h>
#include <pingo/SpriteBuffer.hpp>
#include <pingo/Texture.hpp>

#include <iostream>
#include <complex>
#include <cmath>
#include <boost/thread.hpp>

class FractalRenderer
{
public:
	struct Color
	{
		Color() {}
		Color(double r, double g, double b) : r(r), g(g), b(b) {}
		double r, g, b;

		Color operator * (double factor)
		{
			return Color(r*factor, g*factor, b*factor);
		}

		Color operator + (const Color& other)
		{
			return Color(r + other.r, g + other.g, b + other.b);
		}
	};

	struct Job
	{
		size_t y_start;
		size_t y_count;
		double scale;
		std::complex<double> offset;
	};

	static const int max_iterations = 1000;
	static const int max_colors = 50;

	FractalRenderer(unsigned char* texture_data, Color* color_ramp, double texture_width, double texture_height)
		: texture_data(texture_data), color_ramp(color_ramp), texture_width(texture_width), texture_height(texture_height), job_done(false), running(true)
	{
	}

	typedef boost::lock_guard<boost::mutex> lock_guard;

	void setJob(Job new_job)
	{
		current_job = new_job;
		lock_guard lock(job_mutex);
		job_done = false;
	}

	bool jobDone()
	{
		lock_guard lock(job_mutex);
		return job_done;
	}

	void stop()
	{
		lock_guard lock(running_mutex);
		running = false;
	}

	void operator () ()
	{
		bool _running;

		do
		{
			bool done;
			{
				lock_guard lock(job_mutex);
				done = job_done;
			}

			if (!done)
			{
				for (size_t _y = current_job.y_start; _y < current_job.y_start + current_job.y_count; ++_y)
				{
					for (size_t _x = 0; _x < (size_t)texture_width; ++_x)
					{
						std::complex<double> c(((double)_x / texture_width) * 2.0 - 1.0,
											   ((double)_y / texture_height) * 2.0 - 1.0);

						c *= current_job.scale;
						c += current_job.offset;

						std::complex<double> z(0.0, 0.0);

						double q = (z.real() - 1/4)*(z.real() - 1/4) + z.imag()*z.imag();
						bool inside_cardioid = q * (q + (z.real() - 1/4)) < (1/4)*z.imag()*z.imag();
						bool inside_period2_bulb = (z.real() + 1)*(z.real() + 1) + z.imag()*z.imag() < 1/16;

						size_t i = 0;
						if (!inside_cardioid || !inside_period2_bulb)
						{
							while (i < max_iterations && z.real()*z.real() + z.imag()*z.imag() < 2.0*2.0)
							{
								z = z*z + c;
								i += 1;
							}
						}
						else
						{
							i = max_iterations;
						}

						if (z.real()*z.real() + z.imag()*z.imag() < 2.0*2.0 || i == max_iterations)
							WritePixel(_x, _y, -1, 0, 0.0);
						else
						{
							double s = i + (log(log((double)max_iterations)) - log(log(abs(z))))/log(2.0);

							int first = (int)s;
							int second = first + 1;
							double factor = s - first;
							WritePixel(_x, _y, first, second, factor);
						}
					}
				}

				lock_guard lock(job_mutex);
				job_done = true;
			}
			else
			{
				boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(1));
			}

			lock_guard lock(running_mutex);
			_running = running;
		} while (_running);
	}

private:
	FractalRenderer(FractalRenderer& other);

	Job current_job;
	bool job_done;
	bool running;

	boost::mutex job_mutex, running_mutex;

	double texture_width;
	double texture_height;

	unsigned char* texture_data;
	Color* color_ramp;

	void WritePixel(size_t x, size_t y, int first, int second, double factor)
	{
		size_t offset = y * (size_t)texture_width + x;
		
		Color color;
		if (first == -1)
			color = Color(0, 0, 0);
		else
			color = color_ramp[second % (max_colors*2)] * factor + color_ramp[first % (max_colors*2)] * (1.0 - factor);

		texture_data[offset * 3 + 0] = (unsigned char)(color.r * 255.0);
		texture_data[offset * 3 + 1] = (unsigned char)(color.g * 255.0);
		texture_data[offset * 3 + 2] = (unsigned char)(color.b * 255.0);
	}
};

class Application : public Limbus::OpenglWindow::EventHandler
{
	bool running;

	void onClose(Limbus::OpenglWindow& self)
	{
		running = false;
	}

	Limbus::OpenglWindow window;
	Pingo::SpriteBuffer* sprite_buffer;
	Pingo::Texture* texture;
	unsigned char* texture_data;

	FractalRenderer::Color color_ramp[FractalRenderer::max_colors*2];

public:
	void run()
	{
		running = true;
		window.setCaption("Fractal Demo");
		window.addEventHandler(this);
		window.setWidth(256);
		window.setHeight(256);
		window.create();

		glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
		glDisable( GL_DEPTH_TEST );
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_TEXTURE_2D );

		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glOrtho( 0.0f, window.getWidth(), window.getHeight(), 0.0f, -100.0f, 100.0f );
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();

		texture_data = new unsigned char[window.getWidth() * window.getHeight() * 3];

		texture = new Pingo::Texture();
		texture->loadFromMemory(texture_data, window.getWidth(), window.getHeight(), 3);
		sprite_buffer = new Pingo::SpriteBuffer(texture, 1, true);
		
		sprite_buffer->setWritable(true);
		sprite_buffer->setRectangle(0, 0.0f, 0.0f, (float)window.getWidth(), (float)window.getHeight());
		sprite_buffer->setColor(0, 1.0f, 1.0f, 1.0f, 1.0f);
		sprite_buffer->setTextureRectangle(0, 0.0f, 0.0f, (float)window.getWidth(), (float)window.getHeight());
		sprite_buffer->setWritable(false);

		for (size_t i = 0; i < FractalRenderer::max_colors; ++i)
		{
			float factor = (float)i / FractalRenderer::max_colors;
			float inverse_factor = (float)(FractalRenderer::max_colors - i) / FractalRenderer::max_colors;

			color_ramp[i] = FractalRenderer::Color(sqrt(sqrt(factor)), factor, inverse_factor * 0.5f);
			color_ramp[FractalRenderer::max_colors*2 - 1 - i] = color_ramp[i];
		}

		FractalRenderer worker(texture_data, color_ramp, (double)window.getWidth(), (double)window.getHeight());
		FractalRenderer worker2(texture_data, color_ramp, (double)window.getWidth(), (double)window.getHeight());
		
		FractalRenderer::Job job;
		job.offset = std::complex<double>(+0.001643721971153, +0.822467633298876);
		job.scale = 2.0;
		job.y_start = 0;
		job.y_count = window.getHeight() / 2;

		FractalRenderer::Job job2;
		job2.offset = std::complex<double>(+0.001643721971153, +0.822467633298876);
		job2.scale = 2.0;
		job2.y_start = window.getHeight() / 2;
		job2.y_count = window.getHeight() - job2.y_start;
		
		worker.setJob(job);
		worker2.setJob(job2);
		
		boost::thread worker_thread(boost::ref(worker));
		boost::thread worker_thread2(boost::ref(worker2));

		Limbus::Timer timer;
		while (running)
		{
			window.pollEvents();
			double elapsed = 1.0f + timer.getElapsed() * 0.005;
			
			if (worker.jobDone() && worker2.jobDone())
			{
				texture->update(texture_data);

				job.scale = 1.0 / (elapsed * (1.0 / job.scale));
				if (job.scale < 0.00000000001)
					job.scale = 0.00000000001;
				worker.setJob(job);

				job2.scale = job.scale;
				worker2.setJob(job2);
			}

			sprite_buffer->draw(0, 1, 0, 0);
			window.swapBuffers();
		}

		worker.stop();
		worker2.stop();
		worker_thread.join();
		worker_thread2.join();

		delete sprite_buffer;
		delete texture;
		delete [] texture_data;
	}
};

void main()
{
	Application app;
	app.run();
}
