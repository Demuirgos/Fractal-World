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

void mandelbrot::MainPage::play_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	minP = Windows::Foundation::Point(-2, -2);
	maxP = Windows::Foundation::Point(2, 2);
	this->alphaV->Value = -0.1;
	updateConcurrent();
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
	array_view<int, 1> textureView(len,holder);
	concurrency::parallel_for_each(textureView.extent, [=](index<1> idx) restrict(amp)
		{
				int idnx = idx[0];
				double y_int = idnx / width;
				double x_int = idnx % (int)width;
				double y = (y_int / (height - 1)) * disY + minY;
				double x = (x_int / (width - 1)) * disX + minX;
				auto MandelBrot = [=](double x,double y) restrict(amp)
				{
					int iter = 0;
					double c_x = x;
					double c_y = y;
					double z_x = alpha;
					double z_y = beta;
					while (iter < max && (z_x * z_x) + (z_y * z_y) < lim) {
						double x_temporary = z_x;
						z_x = (z_x * z_x) - (z_y * z_y) + c_x;
						z_y = (2 * x_temporary * z_y) + c_y;
						iter++;
					}
					return iter;
				};
				auto JuliaSet = [=](double x, double y) restrict(amp)
				{
					int iter = 0;
					double c_x = alpha;
					double c_y = beta;
					double z_x = x;
					double z_y = y;
					double total = (c_x * c_x) + (c_y * c_y);
					while (iter < max && total < lim) {
						double x_temporary = z_x;
						z_x = (z_x * z_x) - (z_y * z_y) + c_x;
						z_y = (2 * x_temporary * z_y) + c_y;
						total = (z_x * z_x) + (z_y * z_y);
						iter++;
					}
					return iter;
				};
				auto duckAlgo = [=](double x, double y) restrict(amp) {
					int iter = 0;
					double c_x = x;
					double c_y = y;
					double z_x = alpha;
					double z_y = beta;
					while (z_x * z_x + z_y * z_y < 4 && iter < max) {
						double tempx = z_x;
						z_x = z_x * z_x - z_y * z_y + c_x;
						z_y = 2 * tempx * z_y + c_y; z_y = z_y > 0 ? z_y : -z_y;
						iter++;
					}
					return iter;
				};
				auto BurningShip = [=](double x, double y) restrict(amp) {
					int iter = 0;
					double c_x = x;
					double c_y = y;
					double z_x = x;
					double z_y = y;
					while (z_x * z_x + z_y * z_y < lim && iter < max) {
						double temp_x = z_x;
						z_x = z_x * z_x - z_y * z_y - c_x;
						z_y = 2 * abs(temp_x * z_y) - c_y;
						iter++;
					}
					return iter;
				};
				auto MultiJulia = [=](double x, double y, int n) restrict(amp) {
					double R = 1;
					while (power(R, n) - R <= sqrt(x * x + y * y)) R++;
					double c_x = x;
					double c_y = y;
					double z_x = alpha;
					double z_y = beta;
					int iter = 0;
					while (z_x * z_x + z_y * z_y < R * R  &&  iter < max)
					{
						double xtmp = power((z_x * z_x + z_y * z_y), (n / 2)) * cos(n * atan2(z_y, z_x)) + c_x;
						z_y = power((z_x * z_x + z_y * z_y), (n / 2)) * sin(n * atan2(z_y, z_x)) + c_y;
						z_x = xtmp;

						iter++;
					}
					return iter;
				};
				int reached = 0;
				switch (fractalIndex) {
				case 0:
					reached = MandelBrot(x, y);
					break;
				case 1:
					reached = JuliaSet(x, y);
					break;
				case 2:
					reached = duckAlgo(x, y);
					break;
				case 3:
					reached = MultiJulia(x, y, multiJulia);
					break;
				case 4:
					reached = BurningShip(x, -y);
					break;
				default:
					reached = 0;
				}
				int b = (reached % 16 * 16) ;
				int g = (reached % 8 * 32) ;
				int r = (reached % 4 * 64 ) ;
				int a = 255 & 0x0ff;
				textureView[idnx] = ((a & 0x0ff) << 24) | ((r & 0x0ff) << 16) | ((g & 0x0ff) << 8) | (b & 0x0ff);
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
	Point zoom = Point(disP.X / 50, disP.Y / 50);
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


void mandelbrot::MainPage::Resolution_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
	resP = Windows::Foundation::Point((int)dynamic_cast<Slider^>(sender)->Value, (int)dynamic_cast<Slider^>(sender)->Value);
}


void mandelbrot::MainPage::MandelBrot_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->alphaV->Value = 0; this->betaV->Value = 0;
	this->FractalChosen = 0;
	updateConcurrent();
}

void mandelbrot::MainPage::MultiSets_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->FractalChosen = 3;
	updateConcurrent();
}

void mandelbrot::MainPage::BurningShip_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->alphaV->Value = 0;this->beta = 0;
	this->FractalChosen = 2;
	updateConcurrent();
}


void mandelbrot::MainPage::JuliaSet_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->alphaV->Value = -0.1;this->betaV->Value = 0.35;
	this->FractalChosen = 1;
	updateConcurrent();
}


void mandelbrot::MainPage::Animate_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (dynamic_cast<ToggleSwitch^>(sender)->IsOn) {
		this->Animator->Start();
	}
	else {
		this->Animator->Stop();
	}
}


void mandelbrot::MainPage::OnTick(Platform::Object^ sender, Platform::Object^ args)
{
	if (this->FractalChosen % 2 == 0) {
		Zoom(false);
	}
	else if (this->FractalChosen % 2 == 1) {
		this->alphaV->Value += isReversing?-0.01:0.01;
		if (this->alphaV->Value >= 1) {
			this->isReversing = true;
		}
		else if (this->alphaV->Value <= -1) {
			this->isReversing = false;
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

void mandelbrot::MainPage::Attractors_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->FractalChosen = 4;
	updateConcurrent();
}
