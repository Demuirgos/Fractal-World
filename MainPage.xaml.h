//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace mandelbrot
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();
		property Windows::Foundation::Point disP {
			Windows::Foundation::Point get() { return Windows::Foundation::Point(maxP.X - minP.X, maxP.Y - minP.Y); }
		}
		property int FractalChosen {
			int get() { return this->fractalChosen; }
			void set(int v) { 
				this->fractalChosen = v;
				if(v != 3 ){
					this->Nthpower->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
				} else {
					this->Nthpower->Visibility = Windows::UI::Xaml::Visibility::Visible;
				}
			}
		}
	private:
		int limit = 2;
		int max = 256;
		float zoom = 150;
		int powerMJ = 5;
		double alpha = 0.0;
		double beta = 0.0;
		Windows::Foundation::Point minP;
		Windows::Foundation::Point maxP;
		Windows::Foundation::Point resP;
		bool isRendering = false;
		int fractalChosen = 0;
		Windows::UI::Xaml::DispatcherTimer^ Animator = ref new Windows::UI::Xaml::DispatcherTimer();
		byte* GetPointerToPixelData(Windows::Storage::Streams::IBuffer^ buffer);
		void play_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		/*static int iter(double x, double y, int max, float limit);
		void OnTick(Platform::Object^ sender, Platform::Object^ args);
		void update();*/
		void updateConcurrent();

		//julia specific csts
		bool isReversing = false;

		//Mandel specific csts


		void translate(double dx,double dy);
		void Zoom(bool in);

		void Board_PointerWheelChanged(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
		void Board_ManipulationDelta(Platform::Object^ sender, Windows::UI::Xaml::Input::ManipulationDeltaRoutedEventArgs^ e);
		void MaxIter_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
		void DivergenceLimit_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
		void Grid_SizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);
		void Resolution_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
		void MandelBrot_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void MultiSets_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void BurningShip_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void JuliaSet_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Animate_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnTick(Platform::Object^ sender, Platform::Object^ args);
		void Nthpower_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
		void Alpha_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
		void Beta_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
		void Attractors_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	};
}
