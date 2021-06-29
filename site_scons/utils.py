from SCons import Script
import SCons
from SCons.Variables import Variables as Variables_Base
import os
import sys
import subprocess
import string
from collections import UserDict
import math
import stat
import platform
import rules_checker

def CheckPKGConfig(context, version):
  context.Message( 'Checking for pkg-config... ' )
  ret = context.TryAction('pkg-config --atleast-pkgconfig-version="%s"' % version)[0]
  context.Result( ret )
  return ret

def CheckPKG(context, name):
  context.Message( 'Checking for %s... ' % name )
  ret = context.TryAction('pkg-config --exists "%s"' % name)[0]
  context.Result( ret )
  return ret

def CheckTool(context, tool):
  context.Message( 'Checking for %s... ' % tool )
  (ret, outl) = context.TryAction(tool)
  context.Result( ret )

  env = SCons.Environment.Environment()
  env.ParseConfig(tool)

  dict_env = {}
  dict_env['LIBS'] = env['LIBS']
  dict_env['CXXFLAGS'] = env['CXXFLAGS']
  dict_env['CPPPATH'] = env['CPPPATH']
  return (ret, dict_env)

def Action_swig(target=None, source=None, env=None):
  installed_size = 0
  for src in source:
    installed_size += os.stat(str(src))[6]
  control_info = env['CONTROL_TEMPLATE'] % ( math.ceil(installed_size/1024) )
  f = open(str(target[0]), 'w')
  f.write(control_info)
  f.close()
  os.chmod(str(target[0]), stat.S_IRWXU|stat.S_IROTH|stat.S_IWOTH | stat.S_IRGRP | stat.S_IWGRP)
  print("Generating control file: "+str(target[0]))

def Action_make_control(target=None, source=None, env=None):
  installed_size = 0
  for src in source:
    installed_size += os.stat(str(src))[6]
  control_info = env['CONTROL_TEMPLATE'] % ( math.ceil(installed_size/1024) )
  f = open(str(target[0]), 'w')
  f.write(control_info)
  f.close()
  os.chmod(str(target[0]), stat.S_IRWXU|stat.S_IROTH|stat.S_IWOTH | stat.S_IRGRP | stat.S_IWGRP)
  print("Generating control file: "+str(target[0]))

def Action_make_module_makefile(target=None, source=None, env=None):
  objList = []
  for src in source:
    [headName, tailName] = os.path.split(str(src))
    objList.append(tailName.replace('.c','.o'))

  f = open(str(target[0]), 'w')
  moduleName = env['MODULE_NAME']
  f.write("EXTRA_CFLAGS += -I ./\n")
  f.write("%s-objs := %s\n"%(moduleName, ' '.join(objList)))
  f.write("obj-m := %s.o\n"%(moduleName))
  #f.write("KBUILD_EXTRA_SYMBOLS := ./__out__/rpi/sadc/os/buildroot/buildroot/output/build/linux-3.15.8/Module.symvers\n");
  f.close()
  print("Generating Makefile:"+str(target[0]))


def encrypt_id(hash_string):
  import hashlib
  sha_signature = hashlib.sha256(hash_string.encode()).hexdigest()
  return int(sha_signature[0:8],16)

def Action_check_rules(target, source, env):
  if 'donotblock' in env['RULES']:
      fatalerror = False
  else:
      fatalerror = True
  #print(env['RULES'],fatalerror)
  #print(target)
  error=0
  for x in source:
      rls = rules_checker.rules_checker(str(x), rules=env['RULES'], debug_mode=env['RULES'])
      error += rls.error
  if error and fatalerror:
    return -1
  else:
    for x in target:
      outFile = open(str(x), 'a').close()

def Action_make_autogen_source(target=None, source=None, env=None):
      import json
      import re

      # Open ids database
      updated = False
      if sys.version_info >= (3, 0):
        import dbm
        db = dbm.open("ids.dbm", 'c')
      else:
        import anydbm
        db = anydbm.open("ids.dbm", 'c')

      #build revert db
      db_revert = {}

      if sys.version_info >= (3, 0):
          k = db.firstkey()
          while k != None:
            v = db[k]
            #print(k,v)
            assert(v not in db_revert)
            db_revert[v] = k
            k = db.nextkey(k)
      else:
          for k, v in db.iteritems():
            assert(v not in db_revert)
            db_revert[k] = v

      # Code to build "target" from "source"
      id_list = []
      for src in source:
        namespace = []
        for line in open(str(src), 'r'):
          if "namespace" in line:
            m = re.search(r'namespace(\s+)(?P<namespace>[A-Za-z0-9_-]+)', line)
            if m :
              namespace.append(m.group('namespace'))

          if "E_ID" in line:
            m = re.search(r'(?P<id>E_ID_([A-Z0-9_]+))(\s*)(=*)(\s*)(0x(?P<value>[0-9A-F]+))*(\s*)(,*)(\s*)$', line)
            if m :
                if namespace:
                    id_str = '::'.join(namespace)+'::'+m.group('id')
                else:
                    id_str = m.group('id')

                value = m.group('value')
                #print(db_revert)
                if value:
                    value = '0x'+str(value).lower()
                    print("Fixed ID: "+id_str+" to "+ value)
                    # Check that value of db match id
                    if value in db_revert:
                        if db_revert[value] != id_str:
                            print("ERROR: Forced ID does not match Ids DB",m.group('value'), db[id_str], id_str)
                            sys.exit(-1)
                    else:
                        db_revert[value] = id_str
                        db[id_str] = value
                id_list.append(id_str)

      #f = open(str(target[0]), 'w')
      #f.write("obj-m := %s.o\n"%(moduleName))
      #f.write("KBUILD_EXTRA_SYMBOLS := ./__out__/rpi/sadc/os/buildroot/buildroot/output/build/linux-3.15.8/Module.symvers\n");
      #f.close()

      # Unify list
      id_set = list(set(id_list))
      #print(id_set)



      for id_tmp in id_set:
        if id_tmp not in db:
          e_id = encrypt_id(id_tmp);
          i_offset = 0
          while 1:
            e_id_str = hex(e_id+i_offset)

            # === Hack ===, FIXME, use environment value
            if e_id_str.startswith("0xcae0"):
                print("prefix ID 0xcae0xxxx is reserved to HW, please rename "+id_tmp+"("+ e_id_str+ ")")
                sys.exit(-1)

            if e_id_str not in db:
              updated = True
              db_revert[e_id_str] = id_tmp
              #print(id_str,e_id_str)
              db[id_tmp] = e_id_str
            if id_tmp == db_revert[e_id_str]:
              break
            sys.exit(-1)
            assert(i_offset < 10)
            i_offset = i_offset + 1

      # Dump db in json format
      #if updated or not os.path.exists('ids.json'):
      db_cpy = {}
      if sys.version_info >= (3, 0):
          k = db.firstkey()
          while k != None:
            v = db[k]
            #print(k,v)
            db_cpy[k.decode('utf-8')] = v.decode('utf-8')
            k = db.nextkey(k)
      else:
          for k, v in db.iteritems():
            db_cpy[k.decode('utf-8')] = v.decode('utf-8')

      # Dump database in Json
      with open('ids.json', 'w') as f:
        json.dump(db_cpy, f)

      # Dump database in XML
      with open('ids.xml', 'w') as f:
          f.write('<ids>\n')
          if sys.version_info >= (3, 0):
              k = db.firstkey()
              while k != None:
                #f.write('<{0}>{1}</{0}>\n'.format(str(k),str(v)))
                v = db[k]
                f.write('<id><key>{0}</key><value>{1}</value></id>\n'.format(k.decode("utf-8") ,v.decode("utf-8") ))
                k = db.nextkey(k)
              f.write('</ids>\n')
          else:
              for k, v in db.iteritems():
                  f.write('<id><key>{0}</key><value>{1}</value></id>\n'.format(k.decode("utf-8") ,v.decode("utf-8") ))

      f = open(str(target[0]), 'w')
      f.write("/* MASTER - Autogenerated HEADER */\n")
      header_name = "__AUTOGEN__"+str(source[0]).replace('/','_').replace('-','_').replace('.','_').upper()
      f.write("#ifndef %s\n"%(header_name))
      f.write("#define %s\n"%(header_name))
      namespace_list = None
      for id_tmp in id_set:
        id_split = id_tmp.split("::")
        id = id_split[-1]
        id_namespace_list = id_split[:-1]
        #print(id,id_namespace_list)
        if namespace_list != id_namespace_list:
            if namespace_list:
                f.write("};\n")
                for nsp in namespace_list:
                  f.write("}\n")
            for nsp in id_namespace_list:
              f.write("namespace %s {\n"%(nsp))
            f.write("enum ET_ID {\n")
            namespace_list = id_namespace_list

        f.write("  %-30s=%s,\n"%(id,db_cpy[id_tmp]))
      f.write("};\n")
      for nsp in namespace_list:
        f.write("}\n")
      tmp = (str(source[0]).split(".")[0]).split("/");
      #print(tmp)
      check_header_name = "CHECK_"+"_".join(tmp[-2:]).replace('-','_').upper()
      f.write("#define %s() {\\\n"%(check_header_name))
      for id_tmp in id_set:
        f.write("  if(\"%s\" != CT_HOST::host->f_id_name(%s)) { throw _FATAL << \"Id %s does not match database\"; }\\\n"%(id_tmp,db_cpy[id_tmp],id_tmp))
      f.write("} \n")

      f.write("#endif\n")
      f.close()
      print("Generating API header:"+str(target[0]))


def Action_make_api_source(target=None, source=None, env=None):
      f = open(str(target[0].abspath), 'w')
      f.write("/* MASTER - Autogenerated source */\n")
      f.write("#include <%s>\n"%str(os.path.basename(source[0].abspath)))
      f.close()
      print("Generating API source:"+str(target[0]))

def Action_make_api_interface(target=None, source=None, env=None):
      import re

      # Code to build "target" from "source"
      id_list = []
      for src in source:
        namespace = []
        for line in open(str(src), 'r'):
          if "namespace" in line:
            m = re.search(r'namespace(\s+)(?P<namespace>[A-Za-z0-9_-]+)', line)
            if m :
              if m.group('namespace') not in namespace:
                  namespace.append(m.group('namespace'))
          m = re.search(r'struct(\s+)(?P<id>[A-Z0-9_]+)(\s*)([{:])(\s*)$', line)
          if m :
                print("STRUCT: "+str(m.group('id')))
                id_list.append({'name':m.group('id'), 'namespace':namespace})


      tmp = (str(source[0]).split(".")[0]).split("/");
      module_name = '_'.join(tmp[-2:]).replace('-','_').lower()
      header = os.path.basename(source[0].abspath)
      header_autogen = os.path.basename(source[1].abspath)
      print(header)
      print(header_autogen)
      print(module_name)
      print(str(target[0]))
      f = open(str(target[0]), 'w')
      f.write("/* MASTER - Autogenerated SWIG */\n")
      f.write("%%module %s\n"%(module_name))
      f.write("%{\n")
      f.write("  #include \"%s\"\n"%(header))
      if len(namespace):
          f.write("  using namespace %s;\n"%('::'.join(namespace)))
      f.write("%}\n")
      f.write("%include \"master.i\"\n")
      f.write("%%include \"%s\"\n"%(header))
      f.write("%%include \"%s\"\n"%(header_autogen))

      for id in id_list:
        f.write("MASTER_STRUCT_NS(%s::, %s)\n"%('::'.join(id['namespace']),id['name']));
      f.close()
      print("Generating API interface:"+str(target[0]))

class BuidSystem(UserDict):
  def __init__(self, args, tools=['default']):
    self.init = True
    UserDict.__init__(self)
    self.tools = tools
    self.env = SCons.Environment.Environment(tools = self.tools)
    self.env.Default(None)
    self.cache_file = 'build-setup.conf'
    self.args = args

    # Reset remote dict
    self.remote_dict = {'default':{'PREFIX':'', 'INC':'' , 'CPPDEFINES':[], 'LIBPATH':[]}}




    self['libs'] = {}
    self['modules'] = {}
    self['LD_LIBRARY_PATH'] = []
    self['MODULE_PATH'] = []
    self.env['STATIC_AND_SHARED_OBJECTS_ARE_THE_SAME']=1
    self.env['LIBS'] = []
    self.env['NVCCGPP'] = "g++-5"
    self.env['LINKFLAGS']=['-fPIC']
    self.env['CCFLAGS'] = ['-g','-Og','-Wall','-Werror','-fPIC','-Wfatal-errors', '-pthread']
    self.env['CXXFLAGS'] = ['-std=c++0x', '-fPIC']
    self.env['CPPSUFFIXES'].append('.moc')
    self.env['CPPDEFINES'] = {}
    self.env['ROOT_PATH'] = self.env.Dir('#/').abspath
    #print(self.env['LIBS'])
    #self.env['ENV']['DISPLAY'] = ":0.0"
    #vars = Variables()
             #vars.Add('ARGS', 'extra-run arguments', '')
    #variables = vars,

    self.env_run = SCons.Environment.Environment( tools = ['default'], ENV = os.environ)
    self.env_run['ARGS']=SCons.Script.SConscript.Arguments.get('ARGS','')

    self.env_run['ENV']['LD_LIBRARY_PATH'] = ""
    self.env_run['ENV']['MODULE_PATH'] = ""
    self.env_run['ENV']['IDS_DB_PATH'] = os.path.dirname(self.env.File('#ids.xml').abspath)

    # VALGRIND DEBUG
    config_valgrind = self.args.get('valgrind')
    if config_valgrind != None:
      self.env['CPPDEFINES']['FF_VALGRIND'] = 1
      print("FF_VALGRIND = 1")
    else:
      print("FF_VALGRIND = 0")
    if not config_valgrind:
      config_valgrind = False


    # GPROF DEBUG
    config_gprof = self.args.get('gprof')
    if not config_gprof:
      config_gprof = False
    else:
      self.env['CCFLAGS'].append('-pg')
      self.env['CXXFLAGS'].append('-pg')
      self.env['LINKFLAGS'].append('-pg')

    # SPROF DEBUG
    config_sprof = self.args.get('sprof')
    if not config_sprof:
      config_sprof = False

    # GPROF DEBUG
    config_gprof = self.args.get('gprof')
    if not config_gprof:
      config_gprof = False

    # OPROF DEBUG
    config_oprof = self.args.get('oprof')
    if not config_oprof:
      config_oprof = False

    # OPROF DEBUG
    config_nvprof = self.args.get('nvprof')
    if not config_nvprof:
      config_nvprof = False

    # EFENCE DEBUG
    config_efence = self.args.get('efence')
    if not config_efence:
      config_efence = False

    # PERF DEBUG
    config_perf = self.args.get('perf')
    if not config_perf:
      config_perf = False

    ##self.env_run['ENV']["MALLOC_CHECK_"]="1"
    if config_oprof:
      print("OPROF: ", config_sprof)
      self.env_run['PREARGS'] = "operf --separate-thread"

    elif config_perf:
      print("PERF: ", config_perf)
      self.env_run['PREARGS'] = "perf record -g -F 999"
    elif config_nvprof:
      print("NVPROF: ", config_nvprof)
      self.env_run['PREARGS'] = "/usr/local/cuda/bin/nvprof -f  --cpu-profiling on --export-profile /tmp/nvprof"
    elif config_valgrind == '1':
      print("VALGRIND: ", config_sprof)
      self.env_run['PREARGS'] = 'valgrind --tool=memcheck --num-callers=40 --leak-check=full'
    elif config_efence:
      print("EFENCE: ", config_efence)
      #self.env['CCFLAGS'].append("-lefence")
      #self.env['LINKFLAGS'].append("-lefence")
      #self.env_run['ENV']['LD_PRELOAD']="libefence.so.0.0"
      self.env_run['ENV']['EF_PROTECT_FREE']="1"
      self.env_run['ENV']['EF_PROTECT_BELOW']="1"
      self.env_run['ENV']['EF_ALIGNMENT']="64"
      self.env_run['ENV']['EF_ALIGNMENT']="4096"
      self.env_run['PREARGS']="LD_PRELOAD=libefence.so.0.0"
    else:
      print("STD PREARGS: ",SCons.Script.SConscript.Arguments.get('PREARGS',''))
      self.env_run['PREARGS']=SCons.Script.SConscript.Arguments.get('PREARGS','')

    if not ('IPTARGET' in self.env_run['ENV']):
      self.env_run['ENV']['IPTARGET'] = "192.168.0.1"

    # SPROF ENV Vars
    if config_sprof:
      print("SPROF: ", config_sprof)
      self.env_run['ENV']['LD_PROFILE'] = config_sprof
      self.env_run['ENV']['LD_PROFILE_OUTPUT']= '/tmp/sprof/'

    # SPROF ENV Vars
    if config_gprof:
      self.env['CPPDEFINES']['FF_GPROF'] = 1
      print("GPROF: ", config_gprof)
      self.env_run['ENV']['CPUPROFILE'] = '/tmp/gprof.prof'
      #self.env['LIBS'].append('profiler')

    # os.environ.get('ARGS', '')
    builder = self.env.Builder(action=["sh -c '$PREARGS ${SOURCE} $ARGS'"])
    self.env_run['BUILDERS']['Run'] = builder
    builder = self.env.Builder(action=["gdb --args ${SOURCE} $ARGS"])
    self.env_run['BUILDERS']['Gdb'] = builder
    builder = self.env.Builder(action=["cuda-gdb --args ${SOURCE} $ARGS"])
    self.env_run['BUILDERS']['CudaGdb'] = builder
    builder = self.env.Builder(action=["cuda-memcheck ${SOURCE} $ARGS"])
    self.env_run['BUILDERS']['CudaMemcheck'] = builder
    builder = self.env.Builder(action=["tar xzf $SOURCE -C ${TARGET.dir}", "touch $TARGET"])
    self.env_run['BUILDERS']['UnTarGz'] = builder
    builder = self.env.Builder(action=["tar xjf $SOURCE -C ${TARGET.dir}", "touch $TARGET"])
    self.env_run['BUILDERS']['UnTarBz'] = builder
    builder = self.env.Builder(action=["touch $TARGET", "patch -d ${TARGET.dir} -p1 < $SOURCE"])
    self.env_run['BUILDERS']['Patch'] = builder
    builder = self.env.Builder(action=['dtc -b 0 -V 17 -R 4 -S 0x3000 -I dts -O dtb -o $TARGET -f $SOURCE'])
    self.env_run['BUILDERS']['Dtc'] = builder

    # SWIG builders
    builder = self.env.Builder(action=["swig -c++ -Wall -python $_CPPINCFLAGS -Isite_scons/swig/ -o $TARGET $SOURCE"])
    self.env['BUILDERS']['SwigPython'] = builder
    builder = self.env.Builder(action=["swig -c++ -Wall -octave $_CPPINCFLAGS -Isite_scons/swig/ -o $TARGET $SOURCE"])
    self.env['BUILDERS']['SwigOctave'] = builder
    builder = self.env.Builder(action=["mkoctfile $_CPPINCFLAGS -o $TARGET $SOURCE"])
    self.env['BUILDERS']['Mkoctfile'] = builder
    #builder = self.env.Builder( action = Action_check_rules)
    #self.env['BUILDERS']['CheckRules'] = builder

    remote_dict = {'ipTarget':self.env_run['ENV']['IPTARGET']}
    builder = self.env.Builder(action=["rsync -h --progress ${SOURCES} root@%(ipTarget)s:/root/"%remote_dict, "touch $TARGET"] )
    #builder = self.env.Builder(action=["scp ${SOURCES} root@%(ipTarget)s:/root/"%remote_dict, "touch $TARGET"] )
    self.env_run['BUILDERS']['RemoteDl'] = builder
    builder = self.env.Builder(action=["scp ${SOURCES} root@%(ipTarget)s:/root/master.xml"%remote_dict, "touch $TARGET"] )
    self.env_run['BUILDERS']['RemoteDlCfg'] = builder
    builder = self.env.Builder(action=['ssh -t root@%(ipTarget)s "LD_LIBRARY_PATH=/root MODULE_PATH=/root MASTER_CONFIG=/root/master.xml /root/${SOURCE.file} $ARGS"'%remote_dict, "touch $TARGET"])
    self.env_run['BUILDERS']['RemoteRun'] = builder
    builder = self.env.Builder(action=['ssh -t root@%(ipTarget)s "/sbin/rmmod ${SOURCE.file} & /sbin/insmod /tmp/${SOURCE.file}"'%remote_dict, "touch $TARGET"])
    self.env_run['BUILDERS']['RemoteInsMod'] = builder
    builder = self.env.Builder(action=["ssh root@%(ipTarget)s gdb --args /root/${SOURCE.file} $ARGS"%remote_dict, "touch $TARGET"])
    self.env_run['BUILDERS']['RemoteGdb'] = builder
    builder = self.env.Builder(action=["ssh root@%(ipTarget)s killall ${SOURCE.file}"%remote_dict, "touch $TARGET"])
    self.env_run['BUILDERS']['RemoteKill'] = builder
    builder = self.env.Builder(action=['ssh root@%(ipTarget)s "/sbin/rmmod ${SOURCE.file} & sleep 1 && /sbin/insmod /root/${SOURCE.file}"'%remote_dict, "touch $TARGET"])
    self.env_run['BUILDERS']['RemoteMod'] = builder

    ## Builders Main ENV
    builder = self.env.Builder(action=["/usr/bin/make -j1 HOSTCC='/usr/bin/gcc' HOSTCFLAGS='' ARCH=$CROSS_ARCH CFLAGS_KERNEL='-Os -I$CROSS_TOOLCHAIN_PATH/include --sysroot=$CROSS_TOOLCHAIN_PATH/ -isysroot $CROSS_TOOLCHAIN_PATH' INSTALL_MOD_PATH=$CROSS_ROOTFS_PATH CROSS_COMPILE=$CROSS_PREFIX LDFLAGS='-L$CROSS_TOOLCHAIN_PATH/lib -L$CROSS_TOOLCHAIN_PATH/usr/lib' -C $CROSS_LINUX_PATH M=$ROOT_PATH/${SOURCE.dir} modules"])
    self.env['BUILDERS']['LinuxModuleBuild'] = builder

    ## colorizer
    from colorizer import colorizer
    if (not ('SCONS_NOCOLOR' in self.env_run['ENV'])) and sys.platform.startswith('linux'):

      self.colorizer = colorizer(noColors=False)
    else :
      print("NOCOLOR")
      self.colorizer = colorizer(noColors=True)

    if (not ('SCONS_NOCOLORIZER' in self.env_run['ENV'])):
      self.colorizer.colorize(self.env)
      if 0:
        self.colorizer.colorizeBuilder(self.env,'Moc5','Building Qt meta object',False,self.colorizer.cBlue)
        self.colorizer.colorizeBuilder(self.env,'XMoc5','Building Qt meta object',False,self.colorizer.cBlue)
      #self.colorizer.colorizeBuilder(self.env,'Uic5','Building Qt UI',False, self.colorizer.cBlue)


    # Get sw version
    p = subprocess.Popen("git describe --tags --dirty", shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, close_fds=True)
    self.version = p.stdout.read().strip().decode('utf-8')
    print("VERSION:" + self.version)


    # Get arch
    p = subprocess.Popen("uname -m", shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, close_fds=True)
    self.arch =  p.stdout.read().strip().decode('utf-8')


    #import os
    num_cpu = int(os.environ.get('NUM_CPU', 4))
    self.env.SetOption('num_jobs', num_cpu)
    print("Running with -j", self.env.GetOption('num_jobs'))


    # Platform info
    p = subprocess.Popen("lsb_release -c -s", shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, close_fds=True)
    distname =  p.stdout.read().strip().decode('utf-8')
    self.env['CPPDEFINES']['PLATFORM'+'_'+(distname).upper()] = 1;
  def AddTool(self, name):
    self.env.Tool(name)

  def GetEnvRun(self):
    return self.env_run

  def ConfigureStart(self):
    try:
      os.remove(self.cache_file)
    except:
      print('First configure !')

    self.vars = Variables(self.cache_file, self.args)

    #Get build version
    remote_version = self.args.get('remote')
    if not remote_version:
      remote_version = 'default'
    remote_version = str.strip(remote_version)
    self['remote'] = remote_version

    config_version = self.args.get('config')
    if not config_version:
      config_version = 'default'
    build_version = str.strip(config_version);

    self['config'] = build_version
    self.env['config_def'] = self['config'].replace('-','_')

    # Add dedicated module path
    self['LD_LIBRARY_PATH'].append(os.path.join(os.path.expanduser("~/.master/"),str(self['config']),"libs/"))
    #self['LD_LIBRARY_PATH'].append('/usr/local/Qt-5.9.5/lib')
    self['MODULE_PATH'].append(os.path.join(os.path.expanduser("~/.master/"),str(self['config']),"modules"))

    # Add config and build configuration to file
    self.vars.Add('config', 'Which configuration to build', build_version+' ')
    self.vars.Add('remote', 'Which remote to use', remote_version+' ')
    self.vars.Add('build', 'Build in debug or release', 'debug'+' ')
    self.vars.UpdateSome(self.env, ('config', 'build','remote'))

    self.conf = SCons.SConf.SConfBase(self.env, custom_tests = { 'CheckPKGConfig' : CheckPKGConfig,
                                'CheckPKG' : CheckPKG
      })
    # Require math.h
    if not self.conf.CheckCHeader('math.h'):
      print('Math.h must be installed!')
      sys.exit(1)

    # Require pkg-config
    if not self.conf.CheckPKGConfig('0.15.0'):
      print('pkg-config >= 0.15.0 not found.')
      sys.exit(1)

  def ConfigureAddLib(self, name='', tool='', check=None, flags=''):
    # Require pkg-config
    if not name:
      print('name not specified')
      sys.exit(1)

    self['libs'][name] = { }

    if check:
      if not self.conf.CheckPKG(check):
        print(check + ' not found.')
        sys.exit(1)

    if tool:
      p = subprocess.Popen(tool, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, close_fds=True)
      output = p.stdout.read().decode('utf-8')

      p.wait()
      if p.returncode:
        if p.stderr:
          output_err = p.stderr.read().decode('utf-8')
        else:
          output_err = ''
        print('Tool "'+ tool + '"s return an error:\n'+output+'\n'+output_err)
        sys.exit(1)
      else:
        self.vars.Add(name+'_config', name + '  configuration', output)
        self.vars.UpdateSome(self.env, name+'_config')
        print('Tool flag for ' +name +'... '+ output.replace('\n',' '))
      #print(output)
      #cust_env.ParseConfig('pkg-config --cflags --libs gtk+-3.0')
      #print(cust_env['LIBS'])

    if flags:
      print('Custom flags for ' +name +'... '+ flags)
      self.vars.Add(name+'_config', name + '  configuration', flags+'\n')
      self.vars.UpdateSome(self.env, name+'_config')


  def ConfigureEnd(self):
    self.init = False
    libs = self['libs'].keys()

    # List of library used
    self.vars.Add('libs_list', 'List of used library', ' '.join(libs) + ' ')
    self.vars.UpdateSome(self.env, 'libs_list')

    # Save verson
    self.vars.Save('build-setup.conf', self.env)
    #print(self.env['libs_list'])
    #print(self.env['libs_list'])


    self.conf.Finish()


  def ConfigureRead(self):
    self.init = False
    self.vars = Variables(self.cache_file, self.args)
    self.vars.Add('config', 'Which configuration to build')
    self.vars.Add('remote', 'Which remote to use')
    self.vars.Add('build', 'Build in debug or release')
    self.vars.UpdateSome(self.env, ('config', 'build', 'remote'))

    self.vars.Add('libs_list', 'List of used library')
    self.vars.UpdateSome(self.env, ('libs_list'))

    libs = self.env['libs_list'].split(' ')
    for name in libs:
      if name:
        self.vars.Add(name+'_config', name + '  configuration')
        self.vars.UpdateSome(self.env, name+'_config')

        self.AddExtLib(name = name, config = self.env[name+'_config'], ext = True)
        #print(self.env[name+'_config'])

    self['config'] = str.strip(self.env['config'])
    self['remote'] = str.strip(self.env['remote'])
    self.env['config_def'] = self['config'].replace('-','_')

    if not self['remote']:
      self['remote'] = 'default'

  def AddSubDirs(self,dirs, buildDir):
    for dir in dirs:
      Script.SConscript(
        dir + os.sep + 'SConscript',
        variant_dir = buildDir + os.sep + dir,
        duplicate = 0
      )

  def AddExtLib(self, name='', config='', ext=True):
    self['libs'][name] = { 'name': name ,'type':'library', 'flags':config, 'ext':ext }

  def GetLib(self, name, fatal=True):
    if not (name in self['libs']):
      if fatal:
          print('Unable to find library '+name)
          sys.exit(1)
      return []

    return self['libs'][name]

  def GetModule(self, name):
    if not (name in self['modules']):
      print('Unable to find modules '+name)
      sys.exit(1)

    return self['modules'][name]


  def BuildPackage(self, sw_dict, prog, env):
    DEBNAME = sw_dict['name'] +"-"+self['config']
    version_num = self.version.split('-',1)
    DEBVERSION = version_num[1]
    DEBMAINT = "Johann Baudy <seagnal@seagnal.fr>"
    if self.arch in ['i686', 'i386']:
      DEBARCH = 'i386'
    elif self.arch in ['x86_64', 'amd64']:
      DEBARCH = 'amd64'
    elif self.arch in ['aarch64']:
      DEBARCH = 'arm64'
    else:
      print('Unknown architecture '+ self.arch)
      sys.exit(1)
    DEBDEPENDS = sw_dict['deb']['depends']
    DEBDESC = sw_dict['deb']['description']
    CONTROL_TEMPLATE = """
Package: %s
Priority: extra
Section: misc
Installed-Size: %s
Maintainer: %s
Architecture: %s
Version: %s
Depends: %s
Description: %s

"""
    env['CONTROL_TEMPLATE'] = CONTROL_TEMPLATE % (
    DEBNAME, '%s', DEBMAINT, DEBARCH, DEBVERSION, DEBDEPENDS, DEBDESC)


    debfolder = '%s-%s-%s' % (DEBNAME, DEBVERSION, DEBARCH)
    debfolder_scons = '#%s' % (debfolder)
    debpkg = '%s.deb' % (debfolder_scons)
    debcontrol = os.path.join(debfolder_scons, "DEBIAN/control")




    # The control file should be updated when the SVN version changes
    env.Depends(debcontrol, env.Value(CONTROL_TEMPLATE))

    # add prog


    #uniqify source
    list_tmp = {}
    for install in sw_dict['object']['install']:
      list_tmp[install['source']] = install


    sources = []
    for install in list_tmp.values():
      #print(install)
      if install['type'] == 'user':
        dst_folder = install['target']
      elif install['type'] == 'shared':
        dst_folder = sw_dict['deb']['target_shared']
      elif install['type'] == 'module':
        dst_folder = sw_dict['deb']['target_module']
      elif install['type'] == 'prog':
        dst_folder = sw_dict['deb']['target_prog']
      else:
        print('Unknown type '+install['type'])
        sys.exit(1)

      basename_src = os.path.basename(install['source'])
      src = env.File(install['source'])
      dst = os.path.join(debfolder_scons, dst_folder, basename_src)

      if 'chmod' in install:
          node = env.Command(dst, src, [SCons.Defaults.Copy ('$TARGET','$SOURCE'), SCons.Defaults.Chmod('$TARGET', install['chmod'])])
      else:
          node = env.Command(dst, src, [SCons.Defaults.Copy ('$TARGET','$SOURCE')])

      sources.append(node)


    # We can generate the control file by calling make_control
    env.Command(debcontrol, sources, Action_make_control)

    #
    env.Depends(debpkg, debcontrol)

    # And we can generate the .deb file by calling dpkg-deb
    a = env.Command(debpkg, debcontrol, "fakeroot dpkg-deb -b %s %s" % (debfolder, "$TARGET"))
    if ('DEBKEY' in env['ENV']):
        env.PostAction(a, "dpkg-sig -k %s --sign %s",env['ENV']['DEBKEY'], debpkg)
    return a

  def BuildBindings(self, library):
      lib_app = library['object']['env_app']

      swig_api_cc = lib_app.Command(action = Action_make_api_source,
                        source = [library['object']['api']],
                        target = library['object']['api_autogen_source']
                        )
      if library['swig']:
          swig_api_i = lib_app.Command(library['object']['api_autogen_interface'], library['swig'], SCons.Defaults.Copy ('$TARGET','$SOURCE'))
          lib_app.Depends(swig_api_i, library['object']['api_autogen_header'])
      else:
          swig_api_i = lib_app.Command(action = Action_make_api_interface,
                        source = [library['object']['api'], library['object']['api_autogen_header']],
                        target = library['object']['api_autogen_interface']
                        )


      swig_app = self.env.Clone()
      swig_app['SHLIBPREFIX'] = '_'
      #swig_app['CPPFLAGS'] = ['']
      swig_python = lib_app.SwigPython(source=swig_api_i, target=library['object']['api_autogen_swig_py'])
      swig_octave = lib_app.SwigOctave(source=swig_api_i, target=library['object']['api_autogen_swig_oct'])
      lib_app.Depends(swig_python, "#site_scons/swig/master.i")
      lib_app.Depends(swig_octave, "#site_scons/swig/master.i")

      python_lib = self.GetLib('python')
      library_dict = self.env.ParseFlags(python_lib['flags'])

      print(library['object']['api_autogen_header'])
      inc_path = os.path.join(os.path.dirname(library['object']['api_autogen_header']),'../')
      if 'CPPPATH' not in swig_app:
          swig_app['CPPPATH'] = []
      swig_app['CPPPATH'].insert(0,lib_app.Dir(inc_path).abspath)
      swig_app['CPPPATH'].extend(lib_app['CPPPATH'])
      swig_app.MergeFlags(library_dict)
      lib_python = swig_app.SharedLibrary(source = [swig_python, swig_api_cc], target = library['object']['api_swig_py'])
      lib_octave = swig_app.Mkoctfile(source = swig_octave, target = library['object']['api_swig_oct'])

      lib_app.Alias(library['name']+'.bindings',[lib_python, lib_octave])

  def BuidAutogen(self, library):
      lib_app = library['object']['env_app']
      # Check remote dict created
      if not self['remote'] in library['object']['targets']:
        library['object']['targets'][self['remote']] = { 'deps':[] }

      lib_obj = library['object']['targets'][self['remote']]
      if 'api_autogen_header' not in lib_obj:
          # From 18.04, need to elimate dependecy of autogen.hh on it self. Don't know really why.
          # Maybe autogen.hh include inside api file.
          lib_app.Ignore(library['object']['api_autogen_header'],library['object']['api_autogen_header'])
          # Create autogen.hh header file

          lib_obj['api_autogen_header'] = lib_app.Command(action = Action_make_autogen_source,
                             source = [library['object']['api']],
                             target = library['object']['api_autogen_header'])
          lib_app.SideEffect('#ids.json', lib_obj['api_autogen_header'])
          # Below ignore is due to "api.autogen.hh" include in api source file.
          # We ignore dependency on itself .. Bug from v3.0.1 (18.04)
          lib_app.Ignore(lib_obj['api_autogen_header'], library['object']['api_autogen_header'])


  def BuildModule(self, sw_dict, library, remote_dict, env_app):

    # Check remote dict created
    if not self['remote'] in library['object']['targets']:
      library['object']['targets'][self['remote']] = { 'deps':[] }
    lib_obj = library['object']['targets'][self['remote']]

    # Create lib if missing
    if not ('shared' in lib_obj):
      lib_app = library['object']['env_app']
      # Update cross variable
      lib_app['CC'] = remote_dict['PREFIX']+'gcc'
      lib_app['CXX'] = remote_dict['PREFIX']+'g++'
      lib_app['LINKER'] = remote_dict['PREFIX']+'ld'
      lib_app['AR'] = remote_dict['PREFIX']+'ar'
      lib_app['CPPPATH'].insert(0,remote_dict['INC'])



      # Autogenerate plugin API header
      if library['object']['api']:
        self.BuidAutogen(library)
        #print(lib_obj['api_autogen_header'])
        #lib_source.append(library['object']['api_autogen_header'])
        #print(lib_obj['api_autogen_header'])
        #print(library['object']['api_autogen_header'])

      # for each module add generated folder (__out__) as include
      for source in library['object']['source_list']:
        if ('api_autogen_header' in lib_obj):
          inc_path = os.path.join(os.path.dirname(library['object']['api_autogen_header']),'../')
          lib_app['CPPPATH'].insert(0,lib_app.Dir(inc_path).abspath)
        for module in library['libs']['module']:
          library_module = self.GetLib(module)
          if library_module and library_module['object']['api_autogen_header']:
            self.BuidAutogen(library_module)
            inc_path = os.path.join(os.path.dirname(library_module['object']['api_autogen_header']),'../')
            lib_app['CPPPATH'].insert(0,lib_app.Dir(inc_path).abspath)

      # Build each object separately
      sharedList = []
      sharedCuList = []
      sharedCheckedList = []
      for sources in library['object']['source_list']:

        sharedobjectList = []
        for source in sources:
            #print(source.get_path())
            extfile = os.path.splitext(source.get_path())
            if extfile[1] in ['.cu']:
                print(source.get_path())
                sharedobject = lib_app.SharedObject(source = source, target=source.get_path()+".o")
                print(sharedobject)
            else:
                sharedobject = lib_app.SharedObject(source = source)
            if library['rules']:
                sharedcheckedobject = lib_app.File(source.get_path()+".rc")
                #f = lambda target, source, env: Action_check_rules(target, source, env, library['rules'])
                #print(sharedcheckedobject)
                lib_app_check = lib_app.Clone(RULES = library['rules'])
                lib_app_check.Command(sharedcheckedobject, source.abspath, Action_check_rules)
                sharedCheckedList.append(sharedcheckedobject)
                lib_app_check.Depends(sharedcheckedobject, sharedobject)

            if ('api_autogen_header' in lib_obj):
              lib_app.Depends(sharedobject, library['object']['api_autogen_header'])
            for module in library['libs']['module']:
              library_module = self.GetLib(module)
              if library_module and library_module['object']['api_autogen_header']:
                lib_app.Depends(sharedobject, library_module['object']['api_autogen_header'])
            if extfile[1] in ['.cu']:
                sharedCuList.append(sharedobject)
            else:
                sharedList.append(sharedobject)

      # Build final library from list of shared object
      if len(sharedCuList):
          if 'CUDA_DLINK_MODE' in env_app:
            cuObj = lib_app.Dlink(target=library['object']['target']+'.dlink. o', source = sharedCuList)
            lib_obj['shared'] = lib_app.SharedLibrary(target = library['object']['target'], source = [sharedList, cuObj, sharedCuList])
          else:
            lib_obj['shared'] = lib_app.SharedLibrary(target = library['object']['target'], source = [sharedList, sharedCuList])
      else:
          lib_obj['shared'] = lib_app.SharedLibrary(target = library['object']['target'], source = sharedList)

      lib_app.Depends(lib_obj['shared'], sharedCheckedList)

      lib_app.Alias(library['name'], lib_obj['shared'])
      #_app.Depends(lib_obj['shared'], lib_obj['deps'])

    if not ('translations' in lib_obj):
      # QT Translation
      lib_obj['translations'] = []
      for [tr_src, tr_dst]  in library['object']['translations']:
        res = lib_app.Qm5(target = tr_dst, source = tr_src);
        #print(res)
        lib_obj['translations'].extend(res)

    for node in lib_obj['shared']:
      library_path = node.get_abspath()
      (head_path, tail_path) = os.path.split(library_path)

      # Add path to module path
      self['MODULE_PATH'].append(head_path)
      #print(library_path)
      #print(head_path)
      #print(self['MODULE_PATH'])
      self.env_run['ENV']['MODULE_PATH'] = ';'.join(set(self['MODULE_PATH']))
      # Add path to library loader
      self['LD_LIBRARY_PATH'].append(head_path)
      self.env_run['ENV']['LD_LIBRARY_PATH'] = ':'.join(set(self['LD_LIBRARY_PATH']))
      env_app['EXTRADEPS'].append(node)

      # Add module to install list
      library['object']['install'].append({'type':'module', 'source':node.get_abspath(), 'target': []})



    for node in lib_obj['translations']:
      env_app['EXTRADEPS'].append(node)
      # Add module to install list
      library['object']['install'].append({'type':'module', 'source':node.get_abspath(), 'target': []})

    return library['object']['install']


  def AddSW(self, sw_dict):
    #print(sw_dict)
    # Update sw_dict

    env_app = self.env.Clone() #SCons.Environment.Environment(tools = self.tools)

    print("## Adding " + sw_dict['name'])

    if not ('type' in sw_dict):
      print('SW type not specified')
      sys.exit(1)

    Listify(sw_dict,'sources')
    Listify(sw_dict,'includes')
    Listify(sw_dict,'defines')
    Listify(sw_dict,'modules')

    if not ('libs' in sw_dict):
      sw_dict['libs'] = {}
    Listify(sw_dict['libs'],'shared')
    Listify(sw_dict['libs'],'static')
    Listify(sw_dict['libs'],'module')

    if not ('install' in sw_dict):
      sw_dict['install'] = []

    if not ('deb' in sw_dict):
      sw_dict['deb'] = {}
    if not ('translations' in sw_dict):
      sw_dict['translations'] = []

    if not ('depends' in sw_dict['deb']):
      sw_dict['deb']['depends'] = ""
    if not ('description' in sw_dict['deb']):
      sw_dict['deb']['description'] = "missing ..."
    if not ('target_shared' in sw_dict['deb']):
      sw_dict['deb']['target_shared'] = os.path.join("usr/lib", sw_dict['name'])
    if not ('target_module' in sw_dict['deb']):
      sw_dict['deb']['target_module'] = os.path.join("usr/share", sw_dict['name'])
    if not ('target_prog' in sw_dict['deb']):
      sw_dict['deb']['target_prog'] = "usr/bin"

    if not ('ext' in sw_dict):
      sw_dict['ext'] = False

    if not ('flags' in sw_dict):
      sw_dict['flags'] = []

    if not ('nvcc-flags' in sw_dict):
      sw_dict['nvcc-flags'] = []

    if not ('api' in sw_dict):
      sw_dict['api'] = None

    if not ('swig' in sw_dict):
      sw_dict['swig'] = None

    if not ('rules' in sw_dict):
      sw_dict['rules'] = None

    Listify(sw_dict,'external-deps')
    if not ('external-includes' in sw_dict):
      sw_dict['external-includes'] = sw_dict['includes']
    else:
      Listify(sw_dict,'external-includes')



    sw_dict['object']={ 'targets': {}, 'install':[], 'api_source_list':[] }

    # Create sw environment
    env_app['CPPDEFINES']['CFG_${config_def}'.strip()] = "1"
    env_app['CPPDEFINES']['CFG_PROFILE'] = "${config_def}"
    env_app['CPPDEFINES']['BS_VERSION'] = "'"+self.version+ "'"
    env_app['CPPPATH'] = []
    env_app['LIBPATH'] = []
    env_app['EXTRADEPS'] = []
    env_app['MODULEDEPS'] = []

    # Replace default CPP flags if provided
    if sw_dict['flags']:
      env_app['CPPFLAGS'] = sw_dict['flags']
      env_app['CCFLAGS'] = sw_dict['flags']

    if sw_dict['nvcc-flags']:
        env_app['NVCCGPUFLAGS'] = " ".join(sw_dict['nvcc-flags'])
    else:
        env_app['NVCCGPUFLAGS'] = "--gpu-architecture=sm_50"

    # Build source list
    source_list = []
    for source in sw_dict['sources']:
      if isinstance(source,str) and (source.find('*') != -1):
        #print(source)
        source_list.append(self.env.Glob(source))
      else:
        source_list.append([self.env.File(source)])

    # Build translate list
    translation_list = []
    for tr in sw_dict['translations']:
      if isinstance(tr,str) and (tr.find('*') != -1):
        #print(source)
        translation_list.extend(self.env.Glob(tr))
      else:
        translation_list.append(self.env.File(tr))

    include_list = []
    for include in sw_dict['external-includes']:
      include_list.append(self.env.Dir(include).srcnode())
    sw_dict['external-includes'] = include_list

    if type(sw_dict['defines']) is dict:
      for k,v in sw_dict['defines'].items():
        env_app['CPPDEFINES'][k] = v
    elif type(sw_dict['defines']) is list:
      for k in sw_dict['defines']:
        if k:
          env_app['CPPDEFINES'][k] = 1
    elif sw_dict['defines']:
      env_app['CPPDEFINES'][sw_dict['defines']] = 1

    #print(env_app['CPPDEFINES'])

    for include in sw_dict['includes']:
      # TODO Seems to bug with ../ --- print(self.env.Dir(include).srcnode().abspath)
      env_app['CPPPATH'].append(self.env.Dir(include).srcnode())
    env_app['CPPPATH'].append(env_app.Dir('.'));

    # CROSS COMPILER

    # Check remote list
    if not self['remote'] in self.remote_dict:
      print('Remote '+ self['remote'] + ' not found!')
      sys.exit(1)
    remote_dict = self.remote_dict[self['remote']]



    # PACKAGING
    for install in sw_dict['install']:
      if install['source'].find('*') != -1:
          print(install['source'], os.getcwd())
          for src_tmp in  self.env.Glob(install['source']):
              print("ouou",src_tmp)
              install_tmp = install.copy()
              install_tmp['type'] = 'user'
              install_tmp['source'] = src_tmp.srcnode().get_abspath()
              sw_dict['object']['install'].append(install_tmp)
      else:
          install['type'] = 'user'
          install['source'] = self.env.File(install['source']).srcnode().get_abspath()
          sw_dict['object']['install'].append(install)
    #  env_app.Install(target=dst, source=src)

    # STATIC INCLUDE
    for static in sw_dict['libs']['static']:
      library = self.GetLib(static)

      if not library['ext']:
        print("- " + static + ' (static)')

        # Check remote dict created
        if not self['remote'] in library['object']['targets']:
          library['object']['targets'][self['remote']] = { 'deps':[] }
        lib_obj = library['object']['targets'][self['remote']]

        # Create lib if missing
        if not ('static' in lib_obj):
          lib_app = library['object']['env_app']
          #print(lib_app['CPPDEFINES'])
          # Update cross variable
          lib_app['CC'] = remote_dict['PREFIX']+'gcc'
          lib_app['CXX'] = remote_dict['PREFIX']+'g++'
          lib_app['LINKER'] = remote_dict['PREFIX']+'ld'
          lib_app['AR'] = remote_dict['PREFIX']+'ar'
          lib_app['CPPPATH'].insert(0,remote_dict['INC'])
          lib_app['LIBPATH'].insert(0,remote_dict['LIBPATH'])
          #print(remote_dict['CPPDEFINES'])
          #print(lib_app['CPPDEFINES'])

          lib_app['CPPDEFINES'].update(remote_dict['CPPDEFINES'])
          lib_obj['static'] = lib_app.StaticLibrary(target = library['object']['target'], source = library['object']['source_list'])
          #lib_app.Depends(lib_obj['static'], lib_obj['deps'])

        for node in lib_obj['static']:
          env_app['LIBS'].append(node)
          # Add static lib to install list
          #sw_dict['object']['install'].append({'type':'static', 'src':node, 'dst': []})

        for include in library['external-includes']:
          env_app['CPPPATH'].append(include)

        # Add install list of library
        sw_dict['object']['install'].extend(library['object']['install'])
      else:
        print('External static - Not supported')
        sys.exit(1)

    # SHARED INCLUDE
    for shared in sw_dict['libs']['shared']:
      library = self.GetLib(shared)
      #print(library)
      if not library['ext']:
        print("- " + shared + ' (shared)')

        # Check remote dict created
        if not (self['remote'] in library['object']['targets']):
          library['object']['targets'][self['remote']] = {'deps':[] }
        lib_obj = library['object']['targets'][self['remote']]

        # Create lib if missing
        if not ('shared' in lib_obj):
          lib_app = library['object']['env_app']
          # Update cross variable
          lib_app['CC'] = remote_dict['PREFIX']+'gcc'
          lib_app['CXX'] = remote_dict['PREFIX']+'g++'
          lib_app['LINKER'] = remote_dict['PREFIX']+'ld'
          lib_app['AR'] = remote_dict['PREFIX']+'ar'
          lib_app['CPPPATH'].insert(0,remote_dict['INC'])

          #print(remote_dict['CPPDEFINES'])

          lib_app['CPPDEFINES'].update(remote_dict['CPPDEFINES'])
          if library['object']['source_list']:
              lib_obj['shared'] = lib_app.SharedLibrary(target = library['object']['target'], source = library['object']['source_list'])
          else:
              lib_obj['shared'] = []
          #lib_app.Depends(lib_obj['shared'], lib_obj['deps'])


        for node in lib_obj['shared']:
          library_path = node.get_abspath()
          (head_path, tail_path) = os.path.split(library_path)
          env_app['LIBS'].append(shared)
          env_app['LIBPATH'].append(head_path)
          env_app['EXTRADEPS'].append(node)
          self['LD_LIBRARY_PATH'].append(head_path)
          self.env_run['ENV']['LD_LIBRARY_PATH'] = ':'.join(self['LD_LIBRARY_PATH'])
          # Add shared lib to install list
          sw_dict['object']['install'].append({'type':'shared', 'source':node.get_abspath(), 'target': []})

        for include in library['external-includes']:
          env_app['CPPPATH'].append(include)

        # Add install list of library
        sw_dict['object']['install'].extend(library['object']['install'])
        #print(sw_dict['object']['install'])

      else:
        print("- " + shared + ' (ext-shared)')
        library_dict = self.env.ParseFlags(library['flags'])
        env_app.MergeFlags(library_dict)

        #print(library_dict)
        if type(env_app['CPPDEFINES']) is list:
          defines_dict = {}
          for item in env_app['CPPDEFINES']:
            if type(item) is dict:
              for k,v in item.items():
                defines_dict[k] = v
            elif type(item) is list:
              for k in sw_dict['defines']:
                if k:
                  defines_dict[k] = 1
            else:
              defines_dict[item] = 1


          env_app['CPPDEFINES'] = defines_dict
          #print("OUOU")
          #print(library_dict)

    # MODULE INCLUDE
    for module in sw_dict['libs']['module']:
        # On program, we build modules
        if sw_dict['type'] == 'program':
          library = self.GetLib(module)
          if not library['ext']:
            print("- " + module + ' (module)')

            install_list = self.BuildModule(sw_dict, library, remote_dict, env_app)
            # Add install list of library
            sw_dict['object']['install'].extend(install_list)
            #print(install_list)
          else:
            print('Not supported')
            sys.exit(1)

    # KERNEL - MODULE DEPS
    for module in sw_dict['modules']:
      mod = self.GetModule(module)
      if not library['ext']:
        print("- " + module + ' (kernel-module)')

        module_obj = mod['object']['targets'][self['remote']]

        for node in module_obj['module']:
          #env_app['EXTRADEPS'].append(node)
          env_app['MODULEDEPS'].append(node)

        # Add install list of library
        sw_dict['object']['install'].extend(install_list)
      else:
        print('Not supported')
        sys.exit(1)

    if sw_dict['type'] == 'program':
      target = sw_dict['name']

      # Check remote dict created
      if not (self['remote'] in sw_dict['object']['targets']):
        sw_dict['object']['targets'][self['remote']] = { 'deps':[] }

      prog_obj = sw_dict['object']['targets'][self['remote']]

      if self['remote'] != 'default':
        # Update cross variable
        env_app['CC'] = remote_dict['PREFIX']+'gcc'
        env_app['CXX'] = remote_dict['PREFIX']+'g++'
        env_app['LINKER'] = remote_dict['PREFIX']+'ld'
        env_app['CPPPATH'].insert(0,remote_dict['INC'])
        env_app['LIBPATH'].insert(0,remote_dict['LIBPATH'])
        env_app['CPPDEFINES'].update(remote_dict['CPPDEFINES'])

        # Compile program
        prog_obj['prog'] = env_app.Program(target = target, source = source_list)
        env_app.Depends(prog_obj['prog'], env_app['EXTRADEPS'])
        env_app.Alias(sw_dict['name'],prog_obj['prog'])

        # Dl builder
        dl_target = sw_dict['name']+'.dl'
        dl_node = self.env_run.RemoteDl(target=dl_target, source=[prog_obj['prog'], env_app['EXTRADEPS']])
        env_app.Alias(dl_target,dl_node)
        env_app.AlwaysBuild(dl_node)

        # Dl builder
        dlcfg_target = sw_dict['name']+'.dlcfg'
        dlcfg_node = self.env_run.RemoteDlCfg(target=dlcfg_target, source=[self.env_run['ENV']['MASTER_CONFIG']])
        env_app.Alias(dl_target,dlcfg_node)
        env_app.AlwaysBuild(dlcfg_node)

        # Run builder
        run_target = sw_dict['name']+'.run'
        run_node = self.env_run.RemoteRun(target=run_target, source=prog_obj['prog'])
        env_app.Depends(run_node,[dl_node,dlcfg_node])
        env_app.Alias(run_target,run_node)
        env_app.AlwaysBuild(run_node)

        # Dl mod builder
        dlmod_target = sw_dict['name']+'.dlmod'
        dlmod_node = self.env_run.RemoteDl(target=dlmod_target, source=[env_app['MODULEDEPS']])
        env_app.Alias(dl_target,dlmod_node)
        env_app.AlwaysBuild(dlmod_node)

        # Install Modules
        mod_node = []
        for mod in env_app['MODULEDEPS']:
          #(head_path, tail_path) = os.path.split(mod.abspath)
          run_target = mod.abspath+'.insmod'
          ins_node = self.env_run.RemoteMod(target=run_target, source=mod)
          env_app.Depends(ins_node,[dlmod_node])
          env_app.Depends(run_node,ins_node)
          env_app.AlwaysBuild(ins_node)
          mod_node.append(ins_node)
        env_app.Alias(sw_dict['name']+'.insmod',mod_node)

        # Gdb builder
        gdb_target = sw_dict['name']+'.gdb'
        gdb_node = self.env_run.RemoteGdb(target=gdb_target, source=prog_obj['prog'])
        env_app.Alias(gdb_target,gdb_node)
        env_app.AlwaysBuild(gdb_node)


        # Gdb builder
        kill_target = sw_dict['name']+'.kill'
        kill_node = self.env_run.RemoteKill(target=kill_target, source=prog_obj['prog'])
        env_app.Alias(kill_target,kill_node)
        env_app.AlwaysBuild(kill_node)


      else:
        # Compile program
        prog_obj['prog'] = env_app.Program(target = target, source = source_list)
        env_app.Depends(prog_obj['prog'], env_app['EXTRADEPS'])
        env_app.Alias(sw_dict['name'],prog_obj['prog'])

        #Run builder
        run_target = sw_dict['name']+'.run'
        run_node = self.env_run.Run(target=run_target, source=prog_obj['prog'])
        env_app.Alias(run_target,run_node)
        env_app.AlwaysBuild(run_node)

        #Gdb builder
        gdb_target = sw_dict['name']+'.gdb'
        gdb_node = self.env_run.Gdb(target=gdb_target, source=prog_obj['prog'])
        env_app.Alias(gdb_target,gdb_node)
        env_app.AlwaysBuild(gdb_node)

        #CudaGdb builder
        cudagdb_target = sw_dict['name']+'.cudagdb'
        cudagdb_node = self.env_run.CudaGdb(target=cudagdb_target, source=prog_obj['prog'])
        env_app.Alias(cudagdb_target,cudagdb_node)
        env_app.AlwaysBuild(cudagdb_node)

        #CudaMemchk builder
        cudamemcheck_target = sw_dict['name']+'.cudamemcheck'
        cudamemcheck_node = self.env_run.CudaMemcheck(target=cudamemcheck_target, source=prog_obj['prog'])
        env_app.Alias(cudamemcheck_target,cudamemcheck_node)
        env_app.AlwaysBuild(cudamemcheck_node)



        # Package builder
        for node in prog_obj['prog']:
          sw_dict['object']['install'].append({'source':node.get_abspath(), 'target':'/usr/bin', 'type': 'prog'})

        install_target = sw_dict['name']+'.install'
        install_node = self.BuildPackage(sw_dict, target, env_app)
        env_app.Alias(install_target,install_node)
        env_app.AlwaysBuild(install_node)

    elif sw_dict['type'] == 'library':
      target = sw_dict['name']

      if sw_dict['api']:
          sw_dict['object']['api'] = env_app.File(sw_dict['api']).abspath
          tmp = (str(sw_dict['object']['api']).split(".")[0]).split("/");
          module_name = '_'.join(tmp[-2:]).replace('-','_').lower()
          sw_dict['object']['api_autogen_header'] = env_app.File(sw_dict['api'].split(".")[0]+".autogen.hh").abspath
          sw_dict['object']['api_autogen_source'] = env_app.File(sw_dict['api'].split(".")[0]+".autogen.C").abspath
          sw_dict['object']['api_autogen_interface'] = env_app.File(sw_dict['api'].split(".")[0]+".autogen.i").abspath
          sw_dict['object']['api_autogen_swig_py'] = env_app.File(sw_dict['api'].split(".")[0]+".swig.python.C").abspath
          sw_dict['object']['api_autogen_swig_oct'] = env_app.File(sw_dict['api'].split(".")[0]+".swig.octave.C").abspath
          sw_dict['object']['api_swig_py'] = env_app.File(module_name+".so").abspath
          sw_dict['object']['api_swig_oct'] = env_app.File(module_name+".oct").abspath

      else:
          sw_dict['object']['api'] = None
          sw_dict['object']['api_autogen_header'] = None

      sw_dict['object']['source_list'] = source_list
      sw_dict['object']['env_app'] = env_app
      sw_dict['object']['targets'] = {};
      sw_dict['object']['target'] = env_app.File(sw_dict['name']).abspath #os.path.join(os.getcwd(),sw_dict['name']);
      sw_dict['object']['translations'] = []
      for tr in translation_list:
        #print(tr)
        (head_src, tail_src) = os.path.split(tr.abspath)
        (head_dst, tail_dst) = os.path.split(env_app.File(sw_dict['name']).abspath)
        (base_dst, ext_dst) = os.path.splitext(tail_dst)
        (base_src, ext_src) = os.path.splitext(tail_src)
        sw_dict['object']['translations'].append([tr, env_app.File('lib'+base_dst+'_'+base_src+'.qm')])

      # Insert lib to dictionnary
      self['libs'][target] = sw_dict

      # Case of direct build
      modInstall = sw_dict['name']+'.modInstall'
      if modInstall in SCons.Script.SConscript.CommandLineTargets:
        #print(sw_dict['object']['install'])
        sw_dict['object']['install'] = []
        if not sw_dict['ext']:
          install_list = self.BuildModule(sw_dict, sw_dict, remote_dict, env_app)
          sw_dict['object']['install'].extend(install_list)
        #lib_obj = sw_dict['object']['targets'][sw_dict['remote']]
        #for node in lib_obj['shared']:
        #  sw_dict['object']['install'].append({'source':node.get_abspath(), 'target':'/usr/bin', 'type': 'prog'})
        install_node = self.BuildPackage(sw_dict, target, env_app)

        env_app.Alias(modInstall,install_node)

      # Module bindingscat
      if sw_dict['object']['api']:
          self.BuildBindings(sw_dict)

    elif sw_dict['type'] == 'module':

      # Check remote dict created
      if not (self['remote'] in sw_dict['object']['targets']):
        sw_dict['object']['targets'][self['remote']] = { 'deps':[] }

      module_obj = sw_dict['object']['targets'][self['remote']]

      if self['remote'] != 'default':
        # Update cross variable
        env_app['CROSS_PREFIX'] = remote_dict['PREFIX']
        env_app['CROSS_TOOLCHAIN_PATH'] = remote_dict['TOOLCHAIN_PATH']
        #print(env_app['ENV']['CROSS_TOOLCHAIN_PATH'])
        env_app['CROSS_ROOTFS_PATH'] = remote_dict['ROOTFS_PATH']
        env_app['CROSS_LINUX_PATH'] = remote_dict['LINUX_PATH']
        env_app['CROSS_ARCH'] = remote_dict['ARCH']
        env_app['MODULE_NAME'] = sw_dict['name']
        makefile = "Makefile"

        # Duplicate source
        dest_list = []
        for source in source_list:
          print(source[0])
          [headName, tailName] = os.path.split(str(source[0].abspath))
          dest = "__"+tailName
          print(dest)
          res = env_app.Command(dest, source[0], SCons.Defaults.Copy ('$TARGET','$SOURCE'))
          dest_list.append(res)

        # Generate Makefile
        env_app.Command(makefile, dest_list, Action_make_module_makefile)

        # Compile program
        module_obj['module'] = env_app.LinuxModuleBuild(target = sw_dict['name']+'.ko', source = makefile)
        env_app.Depends(module_obj['module'], dest_list)
        env_app.Alias(sw_dict['name'],module_obj['module'])

        # Insert lib to dictionnary
        self['modules'][sw_dict['name']] = sw_dict
      else:
        print('Host module not supported yet')

    else:
      print('Type not supported')
      sys.exit(1)





  def GetBuild(self):

    return self.env['BUILD']

def Listify(dict, arg):
  if arg in dict:
    if not isinstance(dict[arg],list):
      tmp = dict[arg]
      dict[arg] = [tmp]
  else:
    dict[arg] = []




def GenerateConfigH(target, source, env):
  config_h_defines = env.Dictionary()
  print_config("Generating config.h with the following settings:",
                  config_h_defines.items())
  for a_target, a_source in zip(target, source):
    config_h = file(str(a_target), "w")
    config_h_in = file(str(a_source), "r")
    config_h.write(config_h_in.read() % config_h_defines)
    config_h_in.close()
    config_h.close()

class Variables(Variables_Base):
    def Update(self, env, args=None):
        self.unknown = {}
        Variables_Base.Update(self, env, args)

    def UpdateSome(self, env, keys, args=None):
        # Make sure it is a list
        keys = Script.Flatten(keys)

        # Remember original values of the environment
        original = {}
        nonexist = []

        for option in self.options:
            if not option.key in keys:
                try:
                    original[option.key] = env[option.key]
                except KeyError:
                    nonexist.append(option.key)

        # Update into the real environment
        self.Update(env, args)

        # Restore the values for keys not to be updated
        for key in original:
            env[key] = original[key]

        for key in nonexist:
            if key in env:
                del env[key]
