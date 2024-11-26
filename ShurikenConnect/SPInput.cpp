# include "SPInput.hpp"

void SPInput::sendString(std::string_view startLine)
{
	m_server.send(static_cast<const void*>(startLine.data()), startLine.size());
}

void SPInput::sendHTML()
{
	sendString(m_htmlResponseHeader);
	m_server.send(static_cast<const void*>(m_htmlData.data()), m_htmlData.size());
}

void SPInput::sendEmptyResponse(std::string_view status)
{
	sendString(
		"HTTP/1.0 " + std::string{ status } + "\r\n"
		"Content-Length: 0\r\n"
		"Connection: keep-alive\r\n"
		"\r\n"
	);
}

void SPInput::sendOK()
{
	sendEmptyResponse("200 OK");
}

void SPInput::sendBadRequest()
{
	sendEmptyResponse("400 Bad Request");
}

void SPInput::sendNotFound()
{
	sendEmptyResponse("404 Not Found");
}

void SPInput::sendTeapot()
{
	sendEmptyResponse("418 I'm a teapot");
}

bool SPInput::parseHeader()
{
	const std::string_view buffer{ std::bit_cast<const char*>(m_buffer.data()), m_buffer.size() };

	const auto headerEnd = buffer.find("\r\n\r\n");
	if (headerEnd == std::string_view::npos)
	{
		return false;
	}
	const auto startLineEnd = buffer.find("\r\n");
	const auto headerStart = (startLineEnd + 2);
	const auto bodyStart = (headerEnd + 4);

	m_startLine = buffer.substr(0, startLineEnd);
	{
		const std::string_view startLineView{ m_startLine };
		const auto methodEnd = startLineView.find(' ');
		const auto resourceStart = (methodEnd + 1);
		const auto resourceEnd = startLineView.find(' ', resourceStart);
		m_method = startLineView.substr(0, methodEnd);
		m_resource = startLineView.substr(resourceStart, resourceEnd - resourceStart);
	}

	// Content-Length だけ見る（手抜き）
	const auto lengthStart = buffer.find("Content-Length: ", headerStart);
	if (lengthStart != std::string_view::npos && lengthStart < headerEnd)
	{
		const auto numberStart = (lengthStart + 16);
		const auto numberEnd = buffer.find("\r\n", numberStart);
		m_contentSize = ParseOr<size_t>(Unicode::WidenAscii(buffer.substr(numberStart, (numberEnd - numberStart))), 0);
	}

	m_buffer.pop_front_N(bodyStart);

	return true;
}

bool SPInput::parseJSON(const JSON& data)
{
	try
	{
		const auto width = data[U"width"].get<int32>();
		const auto height = data[U"height"].get<int32>();
		m_screenSize.x = width;
		m_screenSize.y = height;
		for (const auto& [i, event] : data[U"events"])
		{
			const auto type = event[U"type"].get<String>();
			if (type == U"resize")
			{
				m_resized = true;
			}
			else if (type.starts_with(U"touch"))
			{
				Array<SPTouch> touches;
				for (const auto& [j, touch] : event[U"touches"])
				{
					touches.push_back(SPTouch
						{
							.region = Ellipse
							{
								touch[U"x"].get<double>(),
								touch[U"y"].get<double>(),
								touch[U"a"].get<double>(),
								touch[U"b"].get<double>(),
							},
							.angle = Math::ToRadians(touch[U"angle"].get<double>()),
							.force = touch[U"force"].get<float>(),
							.id = touch[U"id"].get<int32>(),
						});
				}
				Array<int32> changedTouches;
				for (const auto& [j, id] : event[U"changedTouches"])
				{
					changedTouches.push_back(id.get<int32>());
				}

				m_touches = std::move(touches);
				if (type == U"touchstart")
				{
					for (const auto id : changedTouches)
					{
						if (const auto it = std::ranges::find(m_endedTouches, id); it != m_endedTouches.end())
						{
							m_endedTouches.erase(it);
						}
						else if (const auto it2 = std::ranges::find(m_canceledTouches, id); it2 != m_canceledTouches.end())
						{
							m_canceledTouches.erase(it2);
						}
						else
						{
							m_startedTouches.push_back(id);
						}
					}
				}
				else if (type == U"touchmove")
				{
					for (const auto id : changedTouches)
					{
						if (m_startedTouches.contains(id))
						{
							// do nothing
						}
						else if (m_movedTouches.contains(id))
						{
							// do nothing
						}
						else
						{
							m_movedTouches.push_back(id);
						}
					}
				}
				else if (type == U"touchend")
				{
					for (const auto id : changedTouches)
					{
						if (const auto it = std::ranges::find(m_startedTouches, id); it != m_startedTouches.end())
						{
							m_startedTouches.erase(it);
						}
						else
						{
							m_endedTouches.push_back(id);
						}
					}
				}
				else if (type == U"touchcancel")
				{
					for (const auto id : changedTouches)
					{
						if (const auto it = std::ranges::find(m_startedTouches, id); it != m_startedTouches.end())
						{
							m_startedTouches.erase(it);
						}
						else
						{
							m_canceledTouches.push_back(id);
						}
					}
				}
			}
		}
	}
	catch (const Error&)
	{
		return false;
	}
	catch (const std::exception&)
	{
		return false;
	}

	return true;
}

bool SPInput::sendResponse()
{
	if (m_buffer.size() < m_contentSize)
	{
		return false;
	}

	if (m_method == "GET")
	{
		if (m_resource == "/")
		{
			sendHTML();
		}
		else if (m_resource == "/teapot")
		{
			sendTeapot();
		}
		else
		{
			sendNotFound();
		}
	}
	else if (m_method == "POST")
	{
		if (m_resource == "/")
		{
			if (parseJSON(JSON::Load(MemoryViewReader{ m_buffer.data(), m_contentSize })))
			{
				sendOK();
			}
			else
			{
				sendBadRequest();
			}
		}
		else
		{
			sendNotFound();
		}
	}
	else
	{
		sendNotFound();
	}

	m_buffer.pop_front_N(m_contentSize);
	m_contentSize = 0;
	m_method = "";
	m_resource = "";

	return false;
}

SPInput::SPInput(uint16 port)
	: m_port{ port }
	, m_htmlData{ Resource(U"index.html") }
{
	m_htmlResponseHeader =
		"HTTP/1.0 200 OK\r\n"
		"Content-Length: " + std::to_string(m_htmlData.size()) + "\r\n"
		"Content-Type: text/html; charset=utf-8\r\n"
		"Connection: keep-alive\r\n"
		"\r\n";

	m_server.startAcceptMulti(port);
}

void SPInput::update()
{
	m_resized = false;
	m_startedTouches.clear();
	m_movedTouches.clear();
	m_endedTouches.clear();
	m_canceledTouches.clear();

	if (m_server.hasSession())
	{
		if (not m_connected)
		{
			m_connected = true;
		}

		if (const size_t size = m_server.available())
		{
			const size_t startPos = m_buffer.size();
			m_buffer.resize(startPos + size);
			m_server.read((m_buffer.data() + startPos), size);

			// HTTP リクエストをパースして HTTP レスポンスを返す
			while (true)
			{
				if (m_method.empty())
				{
					if (not parseHeader())
					{
						break;
					}
				}
				else
				{
					if (not sendResponse())
					{
						break;
					}
				}
			}
		}
	}
	else if (m_connected)
	{
		reset();
	}
}

void SPInput::reset()
{
	m_server.disconnect();

	m_connected = false;
	m_buffer.clear();
	m_contentSize = 0;
	m_method = "";
	m_resource = "";

	m_resized = false;
	m_startedTouches.clear();
	m_movedTouches.clear();
	m_endedTouches.clear();
	m_canceledTouches.clear();

	m_server.startAcceptMulti(m_port);
}

void SPInput::reset(uint16 port)
{
	m_port = port;
	reset();
}

URL SPInput::getURL() const
{
	const auto& addresses = Network::EnumerateIPv4Addresses();
	if (addresses.isEmpty())
	{
		return U"";
	}
	return U"http://{}:{}/"_fmt(addresses[0], m_port);
}

uint16 SPInput::port() const noexcept
{
	return m_port;
}

bool SPInput::connected() const noexcept
{
	return m_connected;
}

Size SPInput::screenSize() const noexcept
{
	return m_screenSize;
}

bool SPInput::resized() const noexcept
{
	return m_resized;
}

const Array<SPTouch>& SPInput::touches() const noexcept
{
	return m_touches;
}

const Array<int32>& SPInput::startedTouches() const noexcept
{
	return m_startedTouches;
}

const Array<int32>& SPInput::movedTouches() const noexcept
{
	return m_movedTouches;
}

const Array<int32>& SPInput::endedTouches() const noexcept
{
	return m_endedTouches;
}

const Array<int32>& SPInput::canceledTouches() const noexcept
{
	return m_canceledTouches;
}
