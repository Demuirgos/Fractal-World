//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//
#include "pch.h"
#include <amp.h>
#include <windows.h>
#include <ppl.h>
#include "MainPage.xaml.h"
#include <robuffer.h>
#include <amp_math.h>

#define TO_XY(idnx, minX, minY, disX, disY, height, width) \
		double y = ((idnx / width) / (height - 1)) * disY + minY, x = ((idnx % (int)width) / (width - 1)) * disX + minX;
#define FROM_XY(x, y, minX, minY, disX, disY, height, width) \
		int idx = (int)(((y - minY) * (height - 1) / disY * width) + ((x - minX) * (width - 1) / disX));
#define ROTATE_BY(n) for(int i =0; i < n; i++) { double t = x; x = -y; y = t; }
#define FLIP(H, V) x = H ? -x : x; y = V ? -y : y; 

using namespace mandelbrot;
using namespace Platform; 
using namespace concurrency::precise_math;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::Graphics::Imaging;
using namespace Windows::UI::Input::Inking;
using namespace Windows::UI::Input;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml::Navigation;
using namespace Concurrency;
using namespace concurrency;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

MainPage::MainPage()
{
	InitializeComponent();
	minP = Windows::Foundation::Point(-2, -2);
	maxP = Windows::Foundation::Point(2, 2);
	resP = Windows::Foundation::Point(800, 800);
	TimeSpan tickDelay = TimeSpan();
	tickDelay.Duration = 2;
	this->Animator->Interval = tickDelay;
	this->Animator->Tick += ref new Windows::Foundation::EventHandler<Platform::Object^>(this, &mandelbrot::MainPage::OnTick);

}


byte* mandelbrot::MainPage::GetPointerToPixelData(IBuffer^ buffer)
{
	Platform::Object^ obj = buffer;
	Microsoft::WRL::ComPtr<IInspectable> insp(reinterpret_cast<IInspectable*>(obj));
	Microsoft::WRL::ComPtr<IBufferByteAccess> bufferByteAccess;
	insp.As(&bufferByteAccess);
	byte* pixels = nullptr;
	bufferByteAccess->Buffer(&pixels);
	return pixels;
}

double power(double d, int n) restrict(amp) {
	double r = 1;
	while (n--) r *= d;
	return r;
}

double abs(double a) restrict(amp) {
	return a > 0 ? a : -a;
}

double norm(double a, double m, double n) restrict(amp) {
	return (a - m) / (m - n);
}

double fmod(double a, int m) restrict(amp) {
	int n = (int)a;
	int r = n % m;
	return (a - n) + r;
}

void mandelbrot::MainPage::play_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->play->IsEnabled = false;
	this->pause->IsEnabled = true;
	if (!this->isAnimating) {
		this->Animator->Start();
		this->isAnimating = true;
	}
}

void mandelbrot::MainPage::updateConcurrent()
{
	double width = this->resP.X, height = this->resP.Y;
	auto wb = ref new Imaging::WriteableBitmap(width, height);
	byte* imageArray = GetPointerToPixelData(wb->PixelBuffer);
	int len = (int)width * height;
	double minX = minP.X,		maxY = maxP.Y,
	       minY = minP.Y,		maxX = maxP.X,
	       disX = disP.X,		disY = disP.Y,
		   lim = this->limit  , max = this->max,
	       alpha = this->alpha, beta = this->beta;
	int fractalIndex = this->FractalChosen;
	int* holder = new int[len]; int multiJulia = this->powerMJ;
	memset(holder, 0, len * sizeof(int));
	array_view<int, 1> textureView(len,holder);
	bool has_red = this->Red->IsChecked->Value,
		has_green = this->Green->IsChecked->Value,
		has_blue = this->Blue->IsChecked->Value,
		is_HSV = this->HSLM->IsChecked->Value;
	int alphaSrc = this->alphaRgb->Value; int alphaVal = rand() % 255;
	int rotations = this->rotation; bool FlipH = this->horizFlip;
									bool FlipV = this->vertFlip;
	concurrency::parallel_for_each(textureView.extent, [=](index<1> idx) restrict(amp)
		{
				int idnx = idx[0]; 
				TO_XY(idnx, minX, minY, disX, disY, height, width);
				auto Color = [](int mode,int reached, bool has_r, bool has_g, bool has_b, int is_a) {
					if (mode == 0) {
						int b = ((has_b ? reached : 0) % 16 * 16), g = ((has_g ? reached : 0) % 8 * 32);
						int r = ((has_r ? reached : 0) %  4 * 64), a = is_a & 0x0ff;
						return ((a & 0x0ff) << 24) | ((r & 0x0ff) << 16) | ((g & 0x0ff) << 8) | (b & 0x0ff);
					} else {
						int H = (reached % 16 * 16) % 360, S = (reached % 8 * 32) % 100, V = (reached % 4 * 64) % 100;
						float s = S / 100;
						float v = V / 100;
						float C = s * v;
						float X = C * (1 - abs(fmod(H / 60.0, 2) - 1));
						float m = v - C;
						float r, g, b;
						if (H >= 0 && H < 60) {
							r = C, g = X, b = 0;
						}
						else if (H >= 60 && H < 120) {
							r = X, g = C, b = 0;
						}
						else if (H >= 120 && H < 180) {
							r = 0, g = C, b = X;
						}
						else if (H >= 180 && H < 240) {
							r = 0, g = X, b = C;
						}
						else if (H >= 240 && H < 300) {
							r = X, g = 0, b = C;
						}
						else {
							r = C, g = 0, b = X;
						}
						int red = (r + m) * 255;
						int green = (g + m) * 255;
						int blue = (b + m) * 255;
						return ((255 & 0x0ff) << 24) | 
							   ((red & 0x0ff) << 16) | 
							   ((green & 0x0ff) << 8)|
							   (blue & 0x0ff);
					}
				};
				double c_x, c_y, z_x, z_y;
				int iter = 0, reached = 0;
				ROTATE_BY(rotations);
				FLIP(FlipH, FlipV)
				if (fractalIndex == 0 || fractalIndex == 1) {
					iter = 0;
					switch (fractalIndex) {
					case 0:
						c_x = x; c_y = y; z_x = alpha; z_y = beta; break;
					case 1:
						c_x = alpha; c_y = beta; z_x = x; z_y = y; break;
					}
					while (iter < max && (z_x * z_x) + (z_y * z_y) < lim) {
						double x_temporary = z_x;
						z_x = (z_x * z_x) - (z_y * z_y) + c_x;
						z_y = (2 * x_temporary * z_y) + c_y;
						iter++;
					}
					reached = iter;
					textureView[idnx] = Color((int)is_HSV,reached, has_red, has_green, has_blue, alphaSrc == 0 ? 255 : alphaSrc == 1 ? reached : alphaVal);
				}
				else if (fractalIndex == 3 || fractalIndex == 4) {
					double R = 1; iter = 0;
					int n = multiJulia;
					while (power(R, n) - R <= sqrt(x * x + y * y)) R++;
					switch (fractalIndex)
					{
					case 3:
						c_x = x; c_y = y; z_x = alpha; z_y = beta; break;
					case 4:
						c_x = alpha; c_y = beta; z_x = x; z_y = y; break;
					}
					while (z_x * z_x + z_y * z_y < R * R && iter < max)
					{
						double xtmp = power((z_x * z_x + z_y * z_y), (n / 2)) * cos(n * atan2(z_y, z_x)) + c_x;
						z_y = power((z_x * z_x + z_y * z_y), (n / 2)) * sin(n * atan2(z_y, z_x)) + c_y;
						z_x = xtmp;
						iter++;
					}
					reached = iter;
					textureView[idnx] = Color((int)is_HSV, reached, has_red, has_green, has_blue, alphaSrc == 0 ? 255 : alphaSrc == 1 ? reached : alphaVal);
				}
				else if (fractalIndex == 5 || fractalIndex == 6) {
					switch (fractalIndex)
					{
					case 5: 
						c_x = x;  c_y = y; z_x = alpha; z_y = beta; break;
					case 6:
						c_x = x; c_y = -y; z_x = x; z_y = -y; break;
					}
					while (z_x * z_x + z_y * z_y < lim && iter < max) {
						double temp_x = z_x;
						z_x = z_x * z_x - z_y * z_y - c_x;
						z_y = 2 * abs(temp_x * z_y) - c_y;
						iter++;
					}
					reached = iter;
					textureView[idnx] = Color((int)is_HSV, reached, has_red, has_green, has_blue, alphaSrc == 0 ? 255 : alphaSrc == 1 ? reached : alphaVal);
				}
		});
	textureView.synchronize();
	for (int i = 0; i < len*4; i+=4) {
		for (int j = 0; j < 4; j++)
			imageArray[i + j] = (holder[i / 4] >> (j * 8));
	}
	this->Board->Source = wb;
	delete[] holder;
}

void mandelbrot::MainPage::translate(double dx, double dy)
{
	double width = this->Board->Width;
	double height = this->Board->Height;
	double dx_ = -dx / ((width - 1)) * this->disP.X;
	double dy_ = -dy / ((height - 1)) * this->disP.Y;
	this->minP.X += dx_; this->minP.Y += dy_;
	this->maxP.X += dx_; this->maxP.Y += dy_;
}

void mandelbrot::MainPage::Zoom(bool in)
{
	Point zoom = Point(disP.X / 200, disP.Y / 200);
	auto BinaryPoint = [](Point p1, Point p2, String^ s) {
		if (s == "Add")
			return Point(p2.X + p1.X, p2.Y + p1.Y);
		else if (s == "Sub")
			return Point(p1.X - p2.X, p1.Y - p2.Y);
	};
	this->minP = BinaryPoint(minP, zoom, in ? "Sub" : "Add");
	this->maxP = BinaryPoint(maxP, zoom, in ? "Add" : "Sub");
}

void mandelbrot::MainPage::Board_PointerWheelChanged(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
	Zoom(e->GetCurrentPoint(this)->Properties->MouseWheelDelta < 0);
	updateConcurrent();
}

void mandelbrot::MainPage::Board_ManipulationDelta(Platform::Object^ sender, Windows::UI::Xaml::Input::ManipulationDeltaRoutedEventArgs^ e)
{
	translate(e->Delta.Translation.X, e->Delta.Translation.Y);
	updateConcurrent();
}


void mandelbrot::MainPage::MaxIter_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
	this->max = dynamic_cast<Slider^>(sender)->Value;
}


void mandelbrot::MainPage::DivergenceLimit_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
	this->limit = dynamic_cast<Slider^>(sender)->Value;
}


void mandelbrot::MainPage::Grid_SizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e)
{
	double oldHeight = this->Board->Height;
	double oldWidth =  this->Board->Width;
	this->Board->Height = this->ActualHeight;
	this->Board->Width = this->ActualWidth;
	updateConcurrent();
}


void mandelbrot::MainPage::MandelBrot_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->alphaV->Value = 0; this->betaV->Value = 0;
	this->FractalChosen = 0;
	updateConcurrent();
}

void mandelbrot::MainPage::JuliaSet_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->alphaV->Value = -0.1;this->betaV->Value = 0.35;
	this->FractalChosen = 1;
	updateConcurrent();
}

void mandelbrot::MainPage::OnTick(Platform::Object^ sender, Platform::Object^ args)
{
	if (this->Animate->IsOn) {
		Zoom(false);
	}
	else {
		if (this->rndAlpha->IsChecked->Value) {
			this->alphaV->Value += isReversingA ? -0.01 : 0.01;
		}
		if (this->rndBeta->IsChecked->Value) {
			this->betaV->Value += isReversingB ? -0.01 : 0.01;
			
		}
		if (!this->rndBeta->IsChecked->Value && !this->rndAlpha->IsChecked->Value) {
			int mode = rand() % 2;
			this->betaV->Value += mode * (isReversingB ? -0.01 : 0.01);
			this->alphaV->Value += mode * (isReversingA ? -0.01 : 0.01);
		}
		
		if (this->betaV->Value >= 1) {
			this->isReversingB = true;
		}
		else if (this->betaV->Value <= -1) {
			this->isReversingB = false;
		}
		
		if (this->alphaV->Value >= 1) {
			this->isReversingA = true;
		}
		else if (this->alphaV->Value <= -1) {
			this->isReversingA = false;
		}
	}
	updateConcurrent();
}


void mandelbrot::MainPage::Nthpower_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
	this->alphaV->Value = 0;this->betaV->Value = 0;
	if (this->FractalChosen == 3) {
		this->powerMJ = dynamic_cast<Slider^>(sender)->Value;
		updateConcurrent();
	}
}

void mandelbrot::MainPage::Beta_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
	this->alpha = dynamic_cast<Slider^>(sender)->Value;
}

void mandelbrot::MainPage::Alpha_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
	this->beta = dynamic_cast<Slider^>(sender)->Value;
}


void mandelbrot::MainPage::Multilia_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->alphaV->Value = 0.564; this->betaV->Value = 0;
	this->FractalChosen = 4;
	updateConcurrent();
}


void mandelbrot::MainPage::Multibrot_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->alphaV->Value = 0; this->betaV->Value = 0;
	this->FractalChosen = 3;
	updateConcurrent();
}


void mandelbrot::MainPage::BurningShip_Click_1(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->FractalChosen = 6;
	updateConcurrent();
}


void mandelbrot::MainPage::DuckFractals_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->alphaV->Value = -0.1; this->betaV->Value = 0.35;
	this->FractalChosen = 5;
	updateConcurrent();
}


void mandelbrot::MainPage::reset_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->play->IsEnabled = true;
	this->pause->IsEnabled = false;
	minP = Windows::Foundation::Point(-2, -2);
	maxP = Windows::Foundation::Point(2, 2);
	this->alphaV->Value = -0.1;
	this->betaV->Value = 0.1;
	this->Animator->Stop();
	this->isAnimating = false;
	updateConcurrent();
}


void mandelbrot::MainPage::ResolutionX_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
	resP = Windows::Foundation::Point((int)dynamic_cast<Slider^>(sender)->Value, resP.Y);
}


void mandelbrot::MainPage::ResolutionY_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
	resP = Windows::Foundation::Point(resP.X, (int)dynamic_cast<Slider^>(sender)->Value);
}


void mandelbrot::MainPage::ratio_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (dynamic_cast<CheckBox^>(sender)->IsChecked->Value) {
		this->Resolution->Visibility = Windows::UI::Xaml::Visibility::Visible;
		this->XYContainer->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	}
	else {
		this->Resolution->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		this->XYContainer->Visibility = Windows::UI::Xaml::Visibility::Visible;
	}
}

void mandelbrot::MainPage::Resolution_ValueChanged_1(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
	double ratioX = (double)resP.X / resP.Y; int wid = (int)(dynamic_cast<Slider^>(sender)->Value* ratioX);
	double ratioY = (double)resP.Y / resP.X; int hei = (int)(dynamic_cast<Slider^>(sender)->Value* ratioY);
	if (this->ResolutionX != nullptr && this->ResolutionY != nullptr)
	{ this->ResolutionX->Value = wid; this->ResolutionY->Value = hei;}
}


void mandelbrot::MainPage::pause_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->play->IsEnabled = true;
	this->pause->IsEnabled = false;
	if (this->isAnimating) {
		this->Animator->Stop();
		this->isAnimating = false;
	}
}


void mandelbrot::MainPage::Zoom_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
	Slider^ holder = dynamic_cast<Slider^>(sender);
	int middle = (holder->Maximum + holder->Minimum) / 2;
	if (holder->Value > middle) {
		Zoom(true);
		updateConcurrent();
	}
	else if (holder->Value < middle) {
		Zoom(false);
		updateConcurrent();
	}
	holder->Value = middle;
}


void mandelbrot::MainPage::alphaRgb_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
	Slider^ holder = dynamic_cast<Slider^>(sender);
	if (holder->Value == 0) {
		holder->Header = "Alpha From : fixed Value 255";
	}
	else if (holder->Value == 1) {
		holder->Header = "Alpha From : Value Iterations";
	}
	else if (holder->Value == -1) {
		holder->Header = "Alpha From : Random Value";
	}
}


void mandelbrot::MainPage::rotator_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
}


void mandelbrot::MainPage::FlipVertical_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->horizFlip = !this->horizFlip;
	updateConcurrent();
}


void mandelbrot::MainPage::FlipHorizontal_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->vertFlip = !vertFlip;
	updateConcurrent();
}


void mandelbrot::MainPage::RotateRight_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->rotation = (++this->rotation) % 4;
	updateConcurrent();
}


void mandelbrot::MainPage::RotateLeft_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->rotation = (--this->rotation) < 0 ? 3 : this->rotation;
	updateConcurrent();
}
