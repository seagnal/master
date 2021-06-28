#include <toto.h>
#include "check_rules_cpp_test.hh"

namespace NS_2 {
void f_func_test17_good(void) {
  uint32_t & ri_var_test18_good = 0;
  uint32_t * pi_var_test19_good;
  uint32_t i_var_test20_good;
}
}

void f_func_test21_good(void) {
  uint32_t & _var_test22_bad = 0;
  uint32_t * _var_test23_bad;
  uint32_t i_var_test24_good;
}

void f_FUNC_test21_bad(void) {
  uint32_t i_var_test24_good;
}

uint32_t & gri_var_test25_good = 0;
uint32_t * gpi_var_test26_good;
uint32_t gi_var_test27_good(0.0);
CT_SOMETHING gc_var_test28_good;
CT_SOMETHING gc_var_test29_good(a,b,c(),d+4);
CT_SOMETHING gc_var_test30_good(6);
class CT_CLASS_TEST_GOOD gc_var_test31_good;
class CT_CLASS_TEST_GOOD gc_var_test32_good;
struct ST_CLASS_TEST_GOOD gs_var_test33_good;
ST_CLASS_TEST_GOOD gs_var_test34_good;


void f_test_func(ST_STRUCT in_s_warn);
void f_test_func(ST_STRUCT *out_ps_good);
void f_test_func(ST_STRUCT &out_rs_good);

void f_func(const ST_TEST & in_rs_const);
void f_func(ST_TEST const & in_rs_const);

/* FIXME: Est-ce que le comportement doit être différent dans ces cas */
void f_func(char in_c_char_good);
void f_func(const char in_c_char_good);
void f_func(const char * in_ps_char);
void f_func(char * const in_ps_char);
void f_func(const char * const in_ps_char);

/* FIXME: Les lignes avec '?' arrêtent l'exécution du script */
int i_value_good = (0<1) ? 1 : 2;



void f_func_test35_good(void) {
  CT_GUARD<uint32_t> pi_var_test_good = 0;
  CT_GUARD<CT_TOTO<uint32_t> > pc_var_test_good = 0;
  map<uint32_t,std::string> m_var_test_good;
  uint32_t i_var_test24_good;

  std::vector<uint16_t> vi_var_test_good;
  uint16_t ai_var_test_good[1];
  uint16_t ai_var_test_good[10];
  uint16_t ai_var_test_good[C_NB_VAR];
  uint16_t AI_var_test_bad[10];
  uint16_t i_var_TEST_bad;
  std::vector<char> vc_var_test_good;
  std::vector<const char *> vstr_var_test_good;
}

class CT_CLASS_TEST_GOOD : public CT_CLASS_TEST2_GOOD {
  uint32_t * _pi_var_test23_good;
  uint32_t * pi_var_test23_bad;
  uint32_t _ai_var_test_bad;
public:

  /* FIXME: Les variables publiques doivent-elle avoir un '_' comme préfixe ? */
  uint32_t * pi_var_test24_good;
  uint32_t * _pi_var_test25_bad;

  void f_func_test_good(float const * in_pf_var_test_good, uint32_t in_sz_var_test_bad, float in_f_var_test_good);
  void f_func_TEST_bad(void);
}


void CT_TOTO::f_func_test4_good(float const * in_pf_var_test_good, uint32_t in_sz_var_test_bad, float in_f_var_test_good) {

}
