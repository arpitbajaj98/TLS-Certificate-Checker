/**
    Example certifcate code
    gcc -o certexample certexample.c -lssl -lcrypto
*/
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define MINKEYSIZE 2048/8
#define CA_CONSTRAINT "CA:FALSE"

char* ext_data(X509_EXTENSION *extension);
char* get_extension_data(const STACK_OF(X509_EXTENSION) *ext_list, 
                    X509 *cert, int NID);

int main(int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr,"ERROR, no path provided\n");
        exit(1);
    }
    char *filepath = argv[1];

    FILE *input;
    FILE *output;
    input = fopen(filepath, "r");
    output = fopen("output.csv", "w");
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    char *test_cert_example;
   
    char correct = '0';

    //const char test_cert_example[] = "./cert-file2.pem";
    BIO *certificate_bio = NULL;
    X509 *cert = NULL;
    X509_NAME *cert_issuer = NULL;
    X509_CINF *cert_inf = NULL;
    STACK_OF(X509_EXTENSION) * ext_list;

    //initialise openSSL
    OpenSSL_add_all_algorithms();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
    while ((read = getline(&line, &len, input)) != -1) {
       
        char *original = malloc(strlen(line));
        strcpy(original, line);

        char *website_nl;
        char *type_delimitter = ",";
        strtok_r(line, type_delimitter, &website_nl);

        // get rid of new line
        char *website = strtok(website_nl, "\r\n");
        test_cert_example = malloc(strlen(line));
        strcpy(test_cert_example, line);
        //printf("%s\n", line);

        fputs(original, output);
    

        //create BIO object to read certificate
        certificate_bio = BIO_new(BIO_s_file());

        //Read certificate into BIO
        if (!(BIO_read_filename(certificate_bio, test_cert_example)))
        {
            fprintf(stderr, "Error in reading cert BIO filename");
            exit(EXIT_FAILURE);
        }
        if (!(cert = PEM_read_bio_X509(certificate_bio, NULL, 0, NULL)))
        {
            fprintf(stderr, "Error in loading certificate");
            exit(EXIT_FAILURE);
        }

        //cert contains the x509 certificate and can be used to analyse the certificate
        
        //*********************
        // Example code of accessing certificate values
        //*********************

        cert_issuer = X509_get_issuer_name(cert);
        char issuer_cn[256] = "Issuer CN NOT FOUND";
        X509_NAME_get_text_by_NID(cert_issuer, NID_commonName, issuer_cn, 256);
        //printf("Issuer CommonName:%s\n", issuer_cn);

        //List of extensions available at https://www.openssl.org/docs/man1.1.0/crypto/X509_REVOKED_get0_extensions.html
        //Need to check extension exists and is not null
        X509_EXTENSION *ex = X509_get_ext(cert, X509_get_ext_by_NID(cert, NID_subject_key_identifier, -1));
        ASN1_OBJECT *obj = X509_EXTENSION_get_object(ex);
        char buff[1024]; 
        OBJ_obj2txt(buff, 1024, obj, 0);
        //printf("Extension:%s\n", buff);

        // get certificate extensions
        cert_inf = cert->cert_info;
        ext_list = cert_inf->extensions;

        // current time 
        time_t rawtime;    
        time(&rawtime);
        localtime (&rawtime);
        ASN1_TIME *current = NULL;
        ASN1_TIME_set(current, rawtime);

        // get before
        ASN1_TIME *before_time = X509_get_notBefore(cert);
        
        // get after
        ASN1_TIME *after_time = X509_get_notAfter(cert);

        // check before time
        int pday, psec;
        ASN1_TIME_diff(&pday, &psec, before_time, current);
        if(pday > 0 || psec > 0){
            printf("Before: FINE\n");
        } else {
            printf("Before: not fine\n");
        }

        // check after time
        ASN1_TIME_diff(&pday, &psec, current, after_time);
        if(pday > 0 || psec > 0){
            printf("After: FINE\n");
        } else {
            printf("After: not fine\n");
        }

        // get common name 
        X509_NAME *common_name = NULL;
        common_name = X509_get_subject_name(cert);
        char domain_cn[256] = "Domain CN NOT FOUND";
        X509_NAME_get_text_by_NID(common_name, NID_commonName, domain_cn, 256);
        if(strcmp(domain_cn, website)==0){
            printf("Common name is fine\n");
        } else {
            printf("Common Name is not fine\n");
        }

        // https://stackoverflow.com/questions/27327365/openssl-how-to-find-out-what
        // -the-bit-size-of-the-public-key-in-an-x509-certifica
        // Get key size
        EVP_PKEY *cert_key = X509_get_pubkey(cert);
        RSA *rsa = EVP_PKEY_get1_RSA(cert_key);
        int key_length = RSA_size(rsa);
        if(key_length >= MINKEYSIZE){
            printf("Key size is fine\n");
        } else {
            printf("Key size is not fine\n");
        }
        RSA_free(rsa);


        // get extension flags for basic constraints 
       
        char *extension_data = get_extension_data(ext_list, cert, NID_basic_constraints);
        printf("%s\n", extension_data);



        // Get key usage data
        extension_data = get_extension_data(ext_list, cert, NID_ext_key_usage);
        printf("%s\n", extension_data);

        //*********************
        // End of Example code
        //*********************
        X509_free(cert);
        BIO_free_all(certificate_bio);
        printf("\n\n\n");
        
        
        
    }
    
    exit(0);
}


char *
get_extension_data(const STACK_OF(X509_EXTENSION) *ext_list, 
                    X509 *cert, int NID) {

    int exists = X509v3_get_ext_by_NID(ext_list, NID, -1);

    X509_EXTENSION *basic = X509_get_ext(cert, exists);
    ASN1_OBJECT *basic_obj = X509_EXTENSION_get_object(basic);
    char basic_buff[1024]; 
    OBJ_obj2txt(basic_buff, 1024, basic_obj, 0);
    printf("Extension = %s\n", basic_buff);
        
    char *extension_data = ext_data(basic);
    return extension_data;

}

char *
ext_data(X509_EXTENSION *extension) {

        BUF_MEM *bptr = NULL;
        char *buf = NULL;
        BIO *bio = BIO_new(BIO_s_mem());
        if (!X509V3_EXT_print(bio, extension, 0, 0))
        {
            fprintf(stderr, "Error in reading extensions");
        } 
        //ASN1_TIME_print(bio, current);

        BIO_flush(bio);
        BIO_get_mem_ptr(bio, &bptr);
        
        
        //bptr->data is not NULL terminated - add null character
        buf = (char *)malloc((bptr->length + 1) * sizeof(char));
        memcpy(buf, bptr->data, bptr->length);
        buf[bptr->length] = '\0';
        
        BIO_free_all(bio);
        return buf;
}