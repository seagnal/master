import os, sys
import re
import subprocess

from reportlab.platypus.doctemplate import SimpleDocTemplate
from reportlab.platypus.flowables import Image
from reportlab.platypus import Paragraph, Spacer, KeepTogether, PageBreak
from reportlab.platypus.tables import Table, TableStyle
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import getSampleStyleSheet
from reportlab.lib.units import inch
from reportlab.lib import colors
from reportlab.lib import utils
from reportlab.lib.units import cm


reg_exp_id = re.compile('([/][*][*])(?P<comment>[^/]*)([/])(\s*)(\n)(\s*)(?P<id>E_ID_[A-Z0-9_]+)(\s*),')
reg_exp_name_space_id = re.compile('([/][*][*])(?P<comment>[^/]*)([/])(\s*)(\n)(\s*)([/][/])(\s*)(?P<namespace>[a-z:]+)::(?P<id>E_ID_[A-Z0-9]+)(\s*),')
reg_exp_name_space_tag = re.compile('([/][*][*])(?P<comment>[^/]*)([/])(\s*)(\n)(\s*)([/][/])(\s*)[<](?P<tag>[a-zA-Z0-9_-]+)[>](\s*)')
reg_exp_name_space_header = re.compile('([/][*][*])(?P<comment>[^/]*)')

reg_exp_id_single = re.compile('(?P<id>E_ID_[A-Z0-9_]+)')
reg_exp_port_single = re.compile('(?P<id>E_PORT_[A-Z0-9_]+) ([(][A-Za-z]+[)])?')

reg_exp_comment = re.compile("[*]\s*([A-Za-z]+)\s*[:]")


reg_exp_namespace = re.compile("namespace (?P<name>\w+) {")

def ParagraphList(list,styles):
    from reportlab.platypus import Paragraph
    if not len(list):
        return '-'
    else:
        line = '<br/>'.join(list)
    return Paragraph(line, styles)

def ParagraphLine(line,styles):
    from reportlab.platypus import Paragraph
    if line:
        line = re.sub(' +', ' ', line)
        line = line.replace('\n','<br/>')
    else:
        line = ' '
    return Paragraph(line, styles)

def encrypt_id(hash_string):
  import hashlib
  sha_signature = hashlib.sha256(hash_string.encode()).hexdigest()
  return sha_signature[0:8]

#(?P<description>[.]*)([*])(\s*)(Port)(\s*)[:]
class api_doc:
    def __init__(self, filename, plugin,test_mode=None, debug_mode=[], output=None):
        self.filename = filename
        print("Doc generator: %s"%(filename))
        self.debug = debug_mode
        self.plugin = plugin
        if output:
            self.outputfile = output
        else:
            self.outputfile = "plugin_api_"+self.plugin.lower()+".pdf"

        # Get sw version
        p = subprocess.Popen("git describe --tags --dirty", shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, close_fds=True)
        self.version = p.stdout.read().strip().decode('utf-8')

        wd = os.getcwd()
        os.chdir(os.path.dirname(os.path.abspath(filename)))
        p = subprocess.Popen("git describe --tags --dirty", shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, close_fds=True)
        self.version_file = p.stdout.read().strip().decode('utf-8')
        os.chdir(wd)
        print("VERSION FILE:" + self.version_file)

    def parse_file(self):
        with open(self.filename, 'r') as f:
            content = f.read()
            #print(content)

            namespaces = []
            res_namespace = reg_exp_namespace.finditer(content)
            for item in res_namespace:
                name = item.group('name')
                if name in namespaces:
                    break
                namespaces.append(name)
            self.namespace = "::".join(namespaces)


            res = reg_exp_name_space_header.finditer(content)
            for item in res:
                if item.group('comment').find("@file") != -1:
                    #print(item.group('comment'))

                    descList =[]
                    for itemDesc in item.group('comment').split('*'):
                        textTmp = itemDesc.strip()
                        textTmp = textTmp.replace('@author','<br/><b>Author:</b>')
                        if len(textTmp):
                            if textTmp[0] != '@':
                                descList.append(textTmp)



                    self.description = '<br/>'.join(descList)


            res_tmp = []

            res = reg_exp_id.finditer(content)
            for item in res:
                res_tmp.append(item)

            res = reg_exp_name_space_id.finditer(content)
            for item in res:
                #print(item)
                res_tmp.append(item)

            res = reg_exp_name_space_tag.finditer(content)
            for item in res:
                #print(item)
                res_tmp.append(item)

            uidMap = {}
            for item in res_tmp:
                #print(item.group('id'))
                #print(item.group('comment'))

                if 'id' in item.groupdict():
                    id = item.group('id').strip()
                    tag = False
                else:
                    id = "&lt;"+item.group('tag').strip()+"&gt;"
                    tag = True


                if 'namespace' in item.groupdict():
                    namespace = item.group('namespace').strip()
                else:
                    namespace = []
                    #sys.exit(-1)

                res_comm = reg_exp_comment.split(item.group('comment'))

                #for line in item.group('comment').split('*'):
                #    for itemx in line.split(':'):
                #        print(len(itemx), itemx)
                #print(len(res_comm), res_comm)
                current_title = 'title'
                dict_id = {}
                for item_com in res_comm:
                    if item_com.lower().strip() in ['description','port','data','childs','parent']:
                        current_title = item_com.lower()
                    else:
                        if current_title not in dict_id:
                            dict_id[current_title] = ""

                        dict_id[current_title] += item_com

                #print(dict_id)

                dict_clean = {}
                for key in dict_id:
                    dict_clean[key] = dict_id[key].strip()

                dict_clean['tag'] = tag

                if 'parent' in dict_clean:
                    dict_clean['parent'] = reg_exp_id_single.findall(dict_clean['parent'])
                else:
                    dict_clean['parent'] = []

                if 'port' in dict_clean:
                    previous = dict_clean['port']
                    dict_clean['port'] = []
                    dict_clean['port_in'] = []
                    dict_clean['port_out'] = []
                    if previous.strip().replace('*','') == "SETTINGS\n":
                        dict_clean['port'].append("SETTINGS")
                        dict_clean['port_in'].append("SETTINGS")

                    for res in reg_exp_port_single.findall(previous):
                         dict_clean['port'].append(res[0])
                         #print(res)
                         if "IN" in res[1]:
                             dict_clean['port_in'].append(res[0])
                         if "OUT" in res[1]:
                             dict_clean['port_out'].append(res[0])
                else:
                    if tag:
                        dict_clean['port'] = ['SETTINGS']
                    else:
                        dict_clean['port'] = ['N/A']
                    dict_clean['port_in'] = ['N/A']
                    dict_clean['port_out'] = ['N/A']

                if 'description' not in dict_clean:
                    dict_clean['description'] = "N/A"
                if 'port' not in dict_clean:
                    dict_clean['port'] = "N/A"
                if 'data' not in dict_clean:
                    dict_clean['data'] = "N/A"
                if id.find('') != -1:
                    dict_clean['namespace'] = namespace
                else:
                    dict_clean['namespace'] = None


                dict_clean['childs'] = []
                dict_clean['uid'] = id
                dict_clean['alone'] = True

                dict_clean['title'] = dict_clean['title'].replace('*','').strip()
                dict_clean['description'] = dict_clean['description'].replace('*','').strip()
                dict_clean['data'] = dict_clean['data'].replace('*','').strip()




                #print(id,dict_clean)

                if id in uidMap:
                    print(id, "already exists")
                    sys.exit(-1)

                uidMap[id] = dict_clean


            f.close()

            for uid in uidMap:
                if 'parent' in uidMap[uid]:
                    for uid_parent in uidMap[uid]['parent']:
                        uidMap[uid_parent]['childs'].append(uid)
                        uidMap[uid]['alone'] = False

            self.portMap = {}
            for uid in uidMap:
                if len(uidMap[uid]['parent']) == 0:
                    for port in uidMap[uid]['port']:
                        self.portMap[port] = []

            self.uidMap = uidMap
            self.render()




    def render_uid(self, uid):
        idDesc = self.uidMap[uid]


        if idDesc['namespace']:
            uid_fixme = idDesc['namespace']+"::"+idDesc['uid']
            uidHEx = encrypt_id(idDesc['namespace']+"::"+uid_fixme)
        else:
            uid_fixme = idDesc['uid'].replace("__FIXME__","").replace("__IGNORE__","")
            uidHEx = encrypt_id(self.namespace+"::"+uid_fixme)

        row =[]

        row.append(Paragraph('<b>'+uid_fixme+'</b><br/><i>'+uidHEx+'</i>',self.styles["TitleUid"]))
        row.append(ParagraphLine("<b>"+idDesc['title']+"</b><br/>"+idDesc['description'],self.styles["BodyText"]))
        #row.append(ParagraphLine(idDesc['port'],styles["BodyText"]))
        row.append(ParagraphLine(idDesc['data'],self.styles["BodyText"]))

        self.dataStyle.append(('BACKGROUND',(self.colInfoStart,self.idrow), (self.colInfoEnd,self.idrow), colors.tan))

        self.data.append(row)
        self.idrow += 1

        if len(idDesc['childs']):
            self.data.append([Paragraph('<b>Childs of '+ uid_fixme +'</b>',self.styles["TitleT"]),
                '',
                ''
                ])
            self.dataStyle.append(('BACKGROUND',(self.colInfoStart,self.idrow), (self.colInfoEnd,self.idrow), colors.darkseagreen))
            self.dataStyle.append(('SPAN', (self.colInfoStart,self.idrow), (self.colInfoEnd,self.idrow)))
            self.idrow += 1

            for uid_child in idDesc['childs']:
                self.render_uid(uid_child)


            self.data.append([Paragraph('<b>End of childs of '+ uid_fixme +'</b>',self.styles["TitleT"]),
                '',
                ''
                ])
            self.dataStyle.append(('BACKGROUND',(self.colInfoStart,self.idrow), (self.colInfoEnd,self.idrow), colors.darkorange))
            self.dataStyle.append(('SPAN', (self.colInfoStart,self.idrow), (self.colInfoEnd,self.idrow)))
            self.idrow += 1

        #print(self.idrow)




    def render(self):
        import time
        from reportlab.lib import colors
        from reportlab.lib.enums import TA_JUSTIFY, TA_CENTER
        from reportlab.lib.pagesizes import A3, landscape, portrait
        from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Image, Table, TableStyle
        from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
        from reportlab.lib.units import inch, mm

        doc = SimpleDocTemplate(self.outputfile,pagesize=landscape(A4),
                                rightMargin=18,leftMargin=18,
                                topMargin=18,bottomMargin=18)

        Story=[]
        formatted_time = time.ctime()

        #im = Image(logo, 2*inch, 2*inch)
        #Story.append(im)

        self.styles=getSampleStyleSheet()
        self.styles.add(ParagraphStyle(name='Justify', alignment=TA_JUSTIFY))
        self.styles.add(ParagraphStyle(name='TitleT', alignment=TA_CENTER ,textColor=colors.white))
        self.styles.add(ParagraphStyle(name='TitleUid', alignment=TA_CENTER ,textColor=colors.black))


        ptext = '<font size=24><b>MASTER Plugin API:</b> %s</font>' % self.plugin
        Story.append(Paragraph(ptext, self.styles["Normal"]))
        Story.append(Spacer(1, 24))
        doc.title = 'MASTER Plugin API - '+ self.plugin
        ptext = '<font size=12><b>Plugin version:</b> %s</font>' % self.version_file
        Story.append(Paragraph(ptext, self.styles["Normal"]))
        Story.append(Spacer(1, 12))
        ptext = '<font size=12><b>Master version:</b> %s</font>' % self.version
        Story.append(Paragraph(ptext, self.styles["Normal"]))
        Story.append(Spacer(1, 12))
        ptext = '<font size=12><b>Namespace:</b> %s</font>' % self.namespace
        Story.append(Paragraph(ptext, self.styles["Normal"]))
        Story.append(Spacer(1, 12))
        ptext = '<font size=12><b>Description:</b> %s</font>' % self.description
        Story.append(Paragraph(ptext, self.styles["Normal"]))
        Story.append(Spacer(1, 12))
        # r_tests = int(100.0*self.nb_of_tests_ok/self.nb_of_tests)
        # r_tasks = int(100.0*self.nb_of_tasks_ok/self.nb_of_tasks)
        # ptext = '<font size=12>%d %% Tests passed, %d %% Tasks done </font>' % (r_tests,r_tasks)
        # Story.append(Paragraph(ptext, styles["Normal"]))
        # Story.append(Spacer(1, 12))
        # if self.filter_version:
        #     ptext = '<font size=12>Version filter: %s </font>' % (self.filter_version)
        #     Story.append(Paragraph(ptext, styles["Normal"]))
        #     Story.append(Spacer(1, 12))
        # if self.version:
        #     ptext = '<font size=12>Version: %s </font>' % (self.version)
        #     Story.append(Paragraph(ptext, styles["Normal"]))
        #     Story.append(Spacer(1, 12))

        # Create return address

        col = {}
        col['uid'] = 0
        col['description'] = 1
        #col['port'] = 2
        col['data'] = 2
        #col['childs'] = 4
        #col['parent'] = 5

        self.colInfoStart = 0
        self.colInfoEnd = col['data']





        colWidths = (
            70*mm,
            #30*mm,
            100*mm,
            #20*mm,
            100*mm
            #40*mm
            )


        for port in self.portMap:
            Story.append(Spacer(1, 40))
            if port != "SETTINGS":
                ptext = '<font size=12><b>Port: %s</b></font>' % port
            else:
                ptext = '<font size=12><b>%s</b></font>' % port
            Story.append(Paragraph(ptext, self.styles["Normal"]))
            Story.append(Spacer(1, 12))
            for direction in ["port_in","port_out"]:
                cnt = 0
                for uid in self.uidMap:
                    if (port in self.uidMap[uid][direction]):
                        if self.uidMap[uid]['alone']:
                           cnt += 1

                if not cnt:
                    continue

                if direction == "port_in":
                    ptext = '<font size=11><b>- INPUT</b></font>'
                    Story.append(Paragraph(ptext, self.styles["Normal"]))
                    Story.append(Spacer(1, 10))
                else:
                    ptext = '<font size=11><b>- OUTPUT</b></font>'
                    Story.append(Paragraph(ptext, self.styles["Normal"]))
                    Story.append(Spacer(1, 10))

                for uid in self.uidMap:
                    if (port in self.uidMap[uid][direction]):
                        if self.uidMap[uid]['alone']:
                            self.idrow = 0
                            self.data= []

                            self.dataStyle= [
                                        # Format Titles
                                        ('ALIGN',(0,0),(-1,1),'CENTER'),
                                        # ('SPAN', (0,0), (colInfoEnd,0)),
                                        # ('SPAN', (colTaskStart,0), (colTaskEnd,0)),
                                        # ('SPAN', (colTestStart,0), (-1,0)),
                                        ('BACKGROUND',(0,0),(-1,1), colors.navy),
                                        ('TEXTCOLOR',(0,0),(-1,1), colors.white),

                                        # Align UID
                                        ('ALIGN',(0,0),(0,-1),'CENTER'),
                                        #('BACKGROUND',(0,2),(0,-1),colors.silver),

                                        # Aling test result
                                        #('ALIGN',(-4,2),(-1,-1),'CENTER'),

                                        # Set Test as Failed by Default
                                        # ('BACKGROUND',(col['task_status'],2),(col['task_status'],-1), colors.firebrick),
                                        # ('BACKGROUND',(col['result'],2),(col['result'],-1), colors.firebrick),

                                        # Global
                                        ('VALIGN',(0,0),(-1,-1),'MIDDLE'),
                                        ('INNERGRID', (0,0), (-1,-1), 0.25, colors.black),
                                        ('BOX', (0,0), (-1,-1), 0.25, colors.black),
                                        ]

                            self.data.append([Paragraph('<b>UID</b>',self.styles["TitleT"]),
                                #Paragraph('<b>Parent</b>',styles["TitleT"]),
                                Paragraph('<b>Description</b>',self.styles["TitleT"]),
                                #Paragraph('<b>Port</b>',styles["TitleT"]),
                                Paragraph('<b>Data</b>',self.styles["TitleT"])
                                #Paragraph('<b>Childs</b>',styles["TitleT"])
                                ])
                            self.idrow += 1

                            self.render_uid(uid)

                            t=Table(self.data, colWidths=colWidths)
                            t.setStyle(TableStyle(self.dataStyle))

                            Story.append(t)
                            Story.append(Spacer(1, 24))
            Story.append(PageBreak())

        doc.build(Story)

if __name__ == "__main__":
    # execute only if run as a script
    import argparse
    parser = argparse.ArgumentParser(description='Parse Api files.')
    parser.add_argument('files',
                        metavar='FILES',
                        nargs='+',
                        help='files to open')
    parser.add_argument('--test', action='store_true')
    parser.add_argument('--plugin', help='plugin name')
    parser.add_argument('--debug')
    args = parser.parse_args()
    #print(args.files)
    #print(args.test)
    #print(args.plugin)


    if not isinstance(args.files, list):
        files = [args.files]
    else:
        files = args.files

    debug = []
    if args.debug:
        debug = args.debug.split(',')

    for file in files:
        r = api_doc(file, args.plugin, test_mode=args.test, debug_mode=debug)
        r.parse_file()
