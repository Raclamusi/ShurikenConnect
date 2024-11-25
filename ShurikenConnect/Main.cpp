# include <Siv3D.hpp> // Siv3D v0.6.15
# include "SPInput.hpp"

void Main()
{
	constexpr uint16 port = 50000;

	SPInput input{ port };

	const auto qr = QR::EncodeText(input.getURL());
	const Texture texture{ QR::MakeImage(qr) };

	while (System::Update())
	{
		ClearPrint();

		input.update();

		Print << U"connected: " << input.connected();
		Print << U"screen size: " << input.screenSize();
		for (const auto& touch : input.touches())
		{
			Print << U"touch pos: " << touch.region.center;
		}

		if (input.resized())
		{
			Scene::Rect().draw(Palette::Yellow);
		}

		texture.drawAt(Scene::CenterF());
	}
}
