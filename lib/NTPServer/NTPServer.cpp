#include <NTPServer.h>

static const int NTP_PACKET_SIZE = 48;

NTPServer::NTPServer(Stream& serial, GPSManager& gpsManager) {
	_serial = &serial;
	_gpsManager = &gpsManager;
	_udp = new AsyncUDP();
	_udp->listen(NTP_PORT);
	_udp->onPacket([=](AsyncUDPPacket packet) {
		if (packet.length() == NTP_PACKET_SIZE) {
			uint32_t micros_rx;
			uint32_t time_rx = (uint32_t)now(micros_rx) + 2208988800; // Unix to NTP offset
			uint32_t lastFix = _gpsManager->lastFix();
			double dispersion = lastFix * 0.0000000001; // approx 1 microsecond per second error
			uint16_t dispersion_n = (uint16_t)dispersion;
			uint16_t dispersion_f = (dispersion - dispersion_n) / 0.0000152587890625;
			uint32_t microsFrac_rx = (micros_rx % 1000000) / 0.00023283064365386962890625;
			AsyncUDPMessage* msg = new AsyncUDPMessage(NTP_PACKET_SIZE);
			if (_gpsManager->validFix() && lastFix <= 3600000) { // check if fix is valid and less than 1 hour old
				//TODO Check for upcoming leap second
				msg->write(0b00100100);   // LI, Version, Mode
				msg->write(1);  // stratum
				msg->write(6);  // polling minimum
				msg->write(-9); // precision

				msg->write(0);  // root delay
				msg->write(0);
				msg->write(1);
				msg->write(0xAE);

				msg->write((dispersion_n >> 8) & 0XFF);  // root dispersion
				msg->write(dispersion_n & 0XFF);
				msg->write((dispersion_f >> 8) & 0XFF);
				msg->write(dispersion_f & 0XFF);

				msg->write(71); //"G";
				msg->write(80); //"P";
				msg->write(83); //"S";
				msg->write(0);  //"0";
				
				uint32_t time_fix = time_rx;
				uint32_t micros_fix = (micros_rx % 1000000) - (lastFix * 1000);
				if (micros_fix < 0) {
					time_fix += floor(micros_fix / 1000000);
					micros_fix %= 1000000;
				}
				uint32_t microsFrac_fix = (micros_fix % 1000000) / 0.00023283064365386962890625;

				//Reference Timestamp
				msg->write((time_fix >> 24) & 0XFF);
				msg->write((time_fix >> 16) & 0XFF);
				msg->write((time_fix >> 8) & 0XFF);
				msg->write(time_fix & 0XFF);
				msg->write((microsFrac_fix >> 24) & 0XFF);
				msg->write((microsFrac_fix >> 16) & 0XFF);
				msg->write((microsFrac_fix >> 8) & 0XFF);
				msg->write(microsFrac_fix & 0XFF);

				//Origin Timestamp
				msg->write(packet.data()[40]);
				msg->write(packet.data()[41]);
				msg->write(packet.data()[42]);
				msg->write(packet.data()[43]);
				msg->write(packet.data()[44]);
				msg->write(packet.data()[45]);
				msg->write(packet.data()[46]);
				msg->write(packet.data()[47]);

				//Receive Timestamp
				msg->write((time_rx >> 24) & 0XFF);
				msg->write((time_rx >> 16) & 0XFF);
				msg->write((time_rx >> 8) & 0XFF);
				msg->write(time_rx & 0XFF);
				msg->write((microsFrac_rx >> 24) & 0XFF);
				msg->write((microsFrac_rx >> 16) & 0XFF);
				msg->write((microsFrac_rx >> 8) & 0XFF);
				msg->write(microsFrac_rx & 0XFF);

				//Transmit Timestamp
				uint32_t micros_tx;
				uint32_t time_tx = (uint32_t)now(micros_tx) + 2208988800; // Unix to NTP offset
				uint32_t microsFrac_tx = (micros_tx % 1000000) / 0.00023283064365386962890625;

				msg->write((time_tx >> 24) & 0XFF);
				msg->write((time_tx >> 16) & 0XFF);
				msg->write((time_tx >> 8) & 0XFF);
				msg->write(time_tx & 0XFF);
				msg->write((microsFrac_tx >> 24) & 0XFF);
				msg->write((microsFrac_tx >> 16) & 0XFF);
				msg->write((microsFrac_tx >> 8) & 0XFF);
				msg->write(microsFrac_tx & 0XFF);
			} else {
				msg->write(0b11100100);   // LI, Version, Mode
				msg->write(16);   // stratum
				msg->write(6);   // polling minimum
				msg->write(-6); // precision

				msg->write(0);  // root delay
				msg->write(0);
				msg->write(1);
				msg->write(0xAE);

				msg->write(0xFF); // root dispersion
				msg->write(0xFF);
				msg->write(0xFF);
				msg->write(0xFF);

				msg->write(71); //"G";
				msg->write(80); //"P";
				msg->write(83); //"S";
				msg->write(0); //"0";

				// Time unknown -- return 0
				//Reference Timestamp
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);

				//Origin Timestamp
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);

				//Receive Timestamp
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);

				//Transmit Timestamp
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);
				msg->write(0);
			}

			packet.send(*msg);
		}
	});
}

NTPServer::~NTPServer() {
	_udp->close();
}