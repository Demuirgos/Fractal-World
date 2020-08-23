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

using namespace mandelbrot;
using namespace Platform;
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
	/*TimeSpan s = TimeSpan();
	s.Duration = 2;
	this->timer->Interval = s;
	this->timer->Tick += ref new Windows::Foundation::EventHandler<Platform::Object^>(this, &mandelbrot::MainPage::OnTick);
	this->timer->Start();*/
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


void mandelbrot::MainPage::play_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	minP = Windows::Foundation::Point(-2, -2);
	maxP = Windows::Foundation::Point(2, 2);
	updateConcurrent();
}

/*
int mandelbrot::MainPage::iter(double x, double y, int max,float limit)
{
	using namespace std;
	int i = 0;
	complex<double> z(x,y);
	complex<double> r(0,0);
	while (i < max && abs(r) < limit) {
		r = r * r + z;
		i++;
	}
	if (x == 0 && y == 0)
		int s = i;
	return i;
}

void mandelbrot::MainPage::OnTick(Platform::Object^ sender, Platform::Object^ args)
{
	updateConcurrent();
}

void mandelbrot::MainPage::update()
{
	double width = this->Board->Width;
	double height = this->Board->Height;
	auto wb = ref new Imaging::WriteableBitmap(width, height);
	byte* imageArray = GetPointerToPixelData(wb->PixelBuffer);
	for (double i = 0; i < height; i++)
	{
		for (double j = 0; j < width; j++) {
			int pos = (i * width + j) * 4;
			double y = (i / (height - 1)) * disP.Y + minP.Y;
			double x = (j / (width - 1)) * disP.X + minP.X;
			int iterPause = iter(x, y, max, limit);
			imageArray[pos] = iterPause % 16 * 16; // Blue
			imageArray[pos + 1] = iterPause % 8 * 32;  // Green
			imageArray[pos + 2] = iterPause % 4 * 64; // Red
			imageArray[pos + 3] = 255; // Alpha
		}

	}

	this->Board->Source = wb;
}
*/

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
	int* holder = new int[len];
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
				int iter = 0;

				double cx = x;
				double cy = y;
				double z_x = 0;
				double z_y = 0;
				double total = (cx * cx) + (cy * cy);
				while (iter < max && total < lim) {
					double x_temporary = z_x;
					z_x = (z_x * z_x) - (z_y * z_y) + cx;
					z_y = (2 * x_temporary * z_y) + cy;
					total = (z_x * z_x) + (z_y * z_y);
					iter++;
				}
				int b = (iter % 16 * 16) ;
				int g = (iter % 8 * 32) ;
				int r = (iter % 4 * 64 ) ;
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

void mandelbrot::MainPage::Board_PointerWheelChanged(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
	auto vect = e->GetCurrentPoint(this)->Properties->MouseWheelDelta;
	Point zoom = Point(disP.X / 50, disP.Y / 50);
	auto BinaryPoint = [](Point p1, Point p2,String^ s) {
		if(s=="Add")
			return Point(p2.X + p1.X, p2.Y + p1.Y);
		else if (s=="Sub")
			return Point(p1.X - p2.X, p1.Y - p2.Y);
	};
	auto zoomIn = vect < 0;
	this->minP = BinaryPoint(minP, zoom, zoomIn?"Sub":"Add");
	this->maxP = BinaryPoint(maxP, zoom, zoomIn?"Add":"Sub");
	updateConcurrent();
}

void mandelbrot::MainPage::Board_ManipulationDelta(Platform::Object^ sender, Windows::UI::Xaml::Input::ManipulationDeltaRoutedEventArgs^ e)
{
	double width = this->Board->Width;
	double height = this->Board->Height;
	double dx = -e->Delta.Translation.X / ((width - 1)) * this->disP.X;
	double dy = -e->Delta.Translation.Y / ((height - 1)) * this->disP.Y;
	this->minP.X += dx; this->minP.Y += dy;
	this->maxP.X += dx; this->maxP.Y += dy;
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
	resP = Windows::Foundation::Point(dynamic_cast<Slider^>(sender)->Value, dynamic_cast<Slider^>(sender)->Value);
}
