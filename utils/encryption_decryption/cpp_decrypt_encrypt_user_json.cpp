
#include <Poco/Crypto/Cipher.h>
#include <Poco/Crypto/CipherFactory.h>
#include <Poco/Crypto/CipherKey.h>
#include <Poco/StreamCopier.h>
#include <Poco/FileStream.h>
#include <Poco/Crypto/CryptoStream.h>
#include <Poco/JSON/Parser.h>
#include <Poco/Base64Decoder.h>
#include <fstream>
#include <cassert>

void print_usage(const std::string& p)
{
        std::cout << "Usage: " + p + " <action> <user json info> "
                "<input file> <output file>\n";
        std::cout << "\twhere <action> is either dec or enc" << std::endl;
}

bool check_arguments(int argc, char** argv)
{
        if (5 > argc) {
		assert(0 < argc);
                print_usage(argv[0]);
                return false;
        }
	std::string action = argv[1];
	if ("dec" != action && "enc" != action) {
		print_usage(argv[0]);
		return false;
	}
	return true;
}

std::string base64_decode(const std::string& str) noexcept
{
        std::istringstream ss(str);
        Poco::Base64Decoder b(ss);
        std::string d((std::istreambuf_iterator<char>(b)),
                        std::istreambuf_iterator<char>());
        return d;
}

Poco::Crypto::Cipher::Ptr create_aes_cipher(const std::string& user_json_info)
{
	try {
		namespace C = Poco::Crypto;
		using P = Poco::JSON::Object::Ptr;
		std::ifstream ifs(user_json_info);
		if (! ifs.is_open()) {
			return nullptr;
		}
		Poco::JSON::Parser parser;
		Poco::Dynamic::Var v = parser.parse(ifs);
		P info = v.extract<P>();
		std::string i = info->getValue<std::string>("aes_iv");
		std::string k = info->getValue<std::string>("aes_key");

		std::string iv = base64_decode(i);
		std::string key = base64_decode(k);
		C::CipherKey::ByteVec iv_vec(iv.begin(), iv.end());
		C::CipherKey::ByteVec key_vec(key.begin(), key.end());
		C::CipherKey aes_key("aes256", key_vec, iv_vec);

		C::CipherFactory& factory = C::CipherFactory::defaultFactory();
		return factory.createCipher(aes_key);
	} catch (Poco::Exception& e) {
		std::cerr << e.displayText() << std::endl;
	}
	return nullptr;
}

int main(int argc, char** argv)
{
	if (! check_arguments(argc, argv)) {
		return 1;
	}
	std::cout << argv[1] << std::endl;
	std::cout << argv[2] << std::endl;
	std::cout << argv[3] << std::endl;
	std::cout << argv[4] << std::endl;

        Poco::Crypto::Cipher::Ptr aes_cipher = create_aes_cipher(argv[2]);
	if (nullptr == aes_cipher) {
		return 1;
	}

	std::string action = argv[1];
	Poco::FileOutputStream decrypt_sink(argv[4]);
	Poco::FileInputStream decrypt_source(argv[3]);
	assert("dec" == action || "enc" == action);
	if ("dec" == action) {
		Poco::Crypto::CryptoInputStream decryptor(decrypt_source,
				aes_cipher->createDecryptor());
		Poco::StreamCopier::copyStream(decryptor, decrypt_sink);
	} else {
		assert("enc" == action);
		Poco::Crypto::CryptoOutputStream encryptor(decrypt_sink,
				aes_cipher->createEncryptor());
		Poco::StreamCopier::copyStream(decrypt_source, encryptor);
	}

        return 0;
}

