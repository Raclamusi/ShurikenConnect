# include <Siv3D.hpp> // Siv3D v0.6.15
# include "SPInput.hpp"

enum class Mode
{
	QRCode,
	Test,
	Game,
};

void Main()
{
	Window::Resize(1000, 600);

	constexpr uint16 DefaultPort = 50000;

	SPInput input{ DefaultPort };

	TextEditState portEditState{ Format(DefaultPort) };

	Mode mode = Mode::QRCode;

	const Font font{ FontMethod::MSDF, 48, Typeface::Bold };
	font.setBufferThickness(3);

	DynamicTexture qrTexture{ QR::MakeImage(QR::EncodeText(input.getURL())) };

	while (System::Update())
	{
		input.update();

		Scene::Rect().right().movedBy(-200, 0).draw(Palette::White);
		if (SimpleGUI::Button(U"Show QR Code", Vec2{ (Scene::Width() - 180), 40}, 160))
		{
			mode = Mode::QRCode;
		}
		if (SimpleGUI::Button(U"Show Touches", Vec2{ (Scene::Width() - 180), 100 }, 160))
		{
			mode = Mode::Test;
		}
		if (SimpleGUI::Button(U"Play Game", Vec2{ (Scene::Width() - 180), 160 }, 160))
		{
			mode = Mode::Game;
		}

		SimpleGUI::Headline(U"Port", Vec2{ (Scene::Width() - 180), 280 }, 160);
		SimpleGUI::TextBox(portEditState, Vec2{ (Scene::Width() - 180), 320 }, 160);
		if (SimpleGUI::Button(U"Reset Port", Vec2{ (Scene::Width() - 180), 380 }, 160))
		{
			input.reset(ParseOr<uint16>(portEditState.text, DefaultPort));
			qrTexture.fill(QR::MakeImage(QR::EncodeText(input.getURL())));
		}

		if (mode == Mode::QRCode)
		{
			font(U"Scan to Connect!").drawAt(48, 400, 60);
			qrTexture.resized(440).draw(Arg::center(400, 330));
		}
		else if (mode == Mode::Test)
		{
			const auto size = input.screenSize();
			if (size.x != 0 && size.y != 0)
			{
				const auto scale = Min((700.0 / size.x), (500.0 / size.y));
				const Vec2 offset{ (400 - size.x * scale * 0.5), (300 - size.y * scale * 0.5) };

				const Transformer2D transformer{ Mat3x2::Scale(scale).translated(offset) };
				
				const RectF screenRect{ size };
				const auto stretch = ((size.x < size.y) ? Vec2{ 5, 30 } : Vec2{ 30, 5 });
				screenRect.stretched(stretch / scale).rounded(20 / scale).draw(Palette::White);
				screenRect.draw(Palette::Black);

				for (const auto& touch : input.touches())
				{
					const Transformer2D touchTransformer{ Mat3x2::Rotate(touch.angle, touch.region.center) };
					touch.region.draw(HSV{ (240 * (1 - touch.force)) });
				}
				for (const auto& touch : input.touches())
				{
					font(touch.id).drawAt(TextStyle::Outline(0.2, Palette::Black), (20 / scale), touch.region.bottom().movedBy(0, 20), Palette::White);
				}
			}
		}
		else if (mode == Mode::Game)
		{

		}
	}
}
