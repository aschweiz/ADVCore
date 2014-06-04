/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ADV_IMAGE_LAYOUT2_H
#define ADV_IMAGE_LAYOUT2_H

namespace AdvLib2
{
	class Adv2ImageLayout {

	public:
		unsigned char LayoutId;
		unsigned int Width;
		unsigned int Height;
		unsigned char Bpp;

		public:
			Adv2ImageLayout();
			~Adv2ImageLayout();
	};
}

#endif //ADV_IMAGE_LAYOUT2_H