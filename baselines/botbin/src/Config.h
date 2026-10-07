#ifndef	CONFIG_H
#define CONFIG_H
#include <string>

using namespace std;
class Config {
public:
	
	string action_ = "";
	string prefix_ = "./dataset";
	string dataset_ = "twitter.txt";
	string dest_ = "./dataset/twitter";
	string src_ = "./dataset/out.twitter";
	string operation_ = "";
	double eps = 0.0;
	int miu = 0;
	int n_;
	void set_file(int _file) {

		// need to set avg & M in calc_k to 
 		if (_file == 0) {
			dest_ = "./dataset/twitter/twitter";
			src_ = "./dataset/twitter/out.twitter";
			n_ = 41652230;
		}
		if (_file == 1) {
			dest_ = "./dataset/pokec/pokec";
			src_ = "./dataset/pokec/out.pokec";
			n_ = 1632803;
		}
		if (_file == 2) {
			dest_ = "./dataset/skitter/skitter";
			src_ = "./dataset/skitter/out.skitter";
			n_ = 1696415;
		}
		if (_file == 3) {
			dest_ = "./dataset/friend/friend";
			src_ = "./dataset/friend/out.friend";
			n_ = 65608367;
		}
		if (_file == 4) {
			dest_ = "./dataset/webbase/webbase";
			src_ = "./dataset/webbase/out.webbase";
			n_ = 115554441;
		}
		if (_file == 5) {
			dest_ = "./dataset/sina/sina";
			src_ = "./dataset/sina/out.sina";
			n_ = 58655849;
		}//sina
		if (_file == 6) {
			dest_ = "./dataset/web12/web12";
			src_ = "./dataset/web12/out.web12";
			n_ = 90320661;
		}
		if (_file == 7) {
			dest_ = "./dataset/brain/brain";
			src_ = "./dataset/brain/out.brain";
			n_ = 784262;
		}
		if (_file == 8) {
			dest_ = "./dataset/orkut/orkut";
			src_ = "./dataset/orkut/out.orkut";
			n_ = 3072441;
		}
		//youtube
		if (_file == 9) {
			dest_ = "./dataset/youtube/youtube";
			src_ = "./dataset/youtube/out.youtube";
			n_ = 3223585;
		}
		if (_file == 10) {
			dest_ = "./dataset/flickr/flickr";
			src_ = "./dataset/flickr/out.flickr";
			//n_ = 41652230;
		}
		if (_file == 11) {
			//pp minier
			dest_ = "./dataset/pp/pp";
			src_ = "./dataset/pp/out.pp";
			n_ = 8254696;
		}
		//lj
		if (_file == 12) {
			dest_ = "./dataset/lj/lj";
			src_ = "./dataset/lj/out.lj";
			n_ = 4847571;
		}
		//topcat
		if (_file == 13) {
			dest_ = "./dataset/topcat/topcat";
			src_ = "./dataset/topcat/out.topcat";
			n_ = 1791489;
		}
	}
};

#endif