
#include <Poco/Crypto/Cipher.h>
#include <Poco/Crypto/CipherFactory.h>
#include <Poco/Crypto/CipherKey.h>
#include <Poco/Crypto/RSAKey.h>
#include <Poco/StreamCopier.h>
#include <Poco/FileStream.h>
#include <Poco/Crypto/CryptoStream.h>
#include <fstream>

void pring_usage()
{
        std::cout << "Usage: a.out <folder of public key> "
                "<input file> <output file>" << std::endl;
}

Poco::Crypto::Cipher::Ptr create_rsa_cipher(const std::string& user_key_path)
{
        Poco::Crypto::CipherFactory &factory =
                Poco::Crypto::CipherFactory::defaultFactory();
        return factory.createCipher(Poco::Crypto::RSAKey(
                        user_key_path + "/public.pem",
                        user_key_path + "/private.pem",
                        "leviathan"));
}

int main(int argc, char** argv)
{
        if (4 > argc) {
                pring_usage();
                return 1;
        }

        Poco::Crypto::Cipher::Ptr rsa_cipher = create_rsa_cipher(argv[1]);
        Poco::FileInputStream encrypt_source(argv[2]);
        Poco::FileOutputStream encrypt_sink(argv[3]);
        Poco::Crypto::CryptoOutputStream encryptor(encrypt_sink,
                        rsa_cipher->createEncryptor());
        Poco::StreamCopier::copyStream(encrypt_source, encryptor);
}

