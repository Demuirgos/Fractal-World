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
	resP = Windows::Foundation::Point(500, 500);
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

void mandelbrot::MainPage::play_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	minP = Windows::Foundation::Point(-2, -2);
	maxP = Windows::Foundation::Point(2, 2);
	this->juliaInit = -0.1;
	updateConcurrent();
}

void mandelbrot::MainPage::updateConcurrent()
{
	double width = this->resP.X;
	double height = this->resP.Y;
	auto wb = ref new Imaging::WriteableBitmap(width, height);
	byte* imageArray = GetPointerToPixelData(wb->PixelBuffer);
	int len = (int)width * height;
	double minX = minP.X; double maxY = maxP.Y;
	double minY = minP.Y; double maxX = maxP.X;
	double disX = disP.X; double disY = disP.Y;
	double lim = this->limit; int max = this->max;
	int fractalIndex = this->fractalChosen;double initjulia = this->juliaInit;
	int* holder = new int[len]; int multiJulia = this->powerMJ;
	array_view<int, 1> textureView(len,holder);
	concurrency::parallel_for_each(textureView.extent, [=](index<1> idx) restrict(amp)
		{
				int idnx = idx[0];
				int z = idnx / (width * height);
				idnx -= (z * width * height);
				double y_int = idnx / width;
				double x_int = idnx % (int)width;
				double y = (y_int / (height - 1)) * disY + minY;
				double x = (x_int / (width - 1)) * disX + minX;
				auto MandelBrot = [=](double x,double y) restrict(amp)
				{
					int iter = 0;
					double c_x = x;
					double c_y = y;
					double z_x = 0;
					double z_y = 0;
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
				auto JuliaSet = [=](double x, double y) restrict(amp)
				{
					int iter = 0;
					double c_x = initjulia;
					double c_y = 0.651;
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
				auto BurningShip = [=](double x, double y) restrict(amp){
					int iter = 0;
					double c_x = x; 
					double c_y = y;
					double z_x = 0; 
					double z_y = 0; 
					while(z_x*z_x + z_y*z_y < 4 && iter < max){
						double temp_x = z_x*z_x - z_y*z_y + c_x;
						z_y = 2 * z_x * z_y + c_y; z_y = z_y > 0 ? z_y : -z_y;
						z_x = temp_x;
						iter++;
					}
					return iter;
				};
				auto MultiJulia = [=](double x, double y, int n) restrict(amp) {
					double R = 1;
					while (power(R, n) - R <= sqrt(x * x + y * y)) R++;
					double c_x = x;
					double c_y = y;
					double z_x = 0;
					double z_y = 0;
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
					reached = BurningShip(x, y);
					break;
				case 3:
					reached = MultiJulia(x, y, multiJulia);
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
	this->fractalChosen = 0;
	updateConcurrent();
}

void mandelbrot::MainPage::MultiSets_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->fractalChosen = 3;
	updateConcurrent();
}

void mandelbrot::MainPage::BurningShip_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->fractalChosen = 2;
	updateConcurrent();
}


void mandelbrot::MainPage::JuliaSet_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->fractalChosen = 1;
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
	if (this->fractalChosen % 2 == 0) {
		Zoom(false);
	}
	else if (this->fractalChosen % 2 == 1) {
		this->juliaInit += isReversing?-0.01:0.01;
		if (this->juliaInit > 1) {
			this->isReversing = true;
		}
		else if (this->juliaInit < -1) {
			this->isReversing = false;
		}
	}
	updateConcurrent();
}


void mandelbrot::MainPage::Nthpower_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
	if (this->fractalChosen == 3) {
		this->powerMJ = dynamic_cast<Slider^>(sender)->Value;
		updateConcurrent();
	}
}
