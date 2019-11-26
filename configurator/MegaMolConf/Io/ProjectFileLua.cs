﻿using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;

namespace MegaMolConf.Io {

    /// <summary>
    /// Io class for project files version 1.0
    /// </summary>
    class ProjectFileLua : ProjectFile {

        /// <summary>
        /// Escapes paths for Lua
        /// </summary>
        internal static string SafeString(string p) {
            return p.Replace(@"\", @"\\").Replace("\"", "\\\"");
        }

        public override void Save(string filename) {
            using (StreamWriter w = new StreamWriter(filename, false, Encoding.ASCII)) {
                if (!string.IsNullOrEmpty(GeneratorComment)) {
                    w.WriteLine("--[[ " + GeneratorComment + " --]]");
                    w.WriteLine();
                }
                if (!string.IsNullOrEmpty(StartComment)) {
                    w.WriteLine("--[[ " + StartComment + " --]]");
                    w.WriteLine();
                }

                if (Views != null) {
                    foreach (View v in Views) {
                        if (v.ViewModule == null) {
                            MessageBox.Show("Cannot save project without view module");
                            return;
                        }
                        w.WriteLine("mmCreateView(\"" + v.Name + "\", \"" + v.ViewModule.Class + "\", \"" + v.ViewModule.Name + "\")" + " --confPos=" + v.ViewModule.ConfPos.ToString());

                        if (v.Modules != null) {
                            foreach (Module m in v.Modules) {
                                string modFullName = "::" + v.Name + "::" + m.Name;
                                if (m.Name != v.ViewModule.Name) {
                                    if (!m.ConfPos.IsEmpty)
                                    {
                                        w.WriteLine("mmCreateModule(\"" + m.Class + "\", \"" + modFullName + "\")" + " --confPos=" + m.ConfPos.ToString());
                                    } else
                                    {
                                        w.WriteLine("mmCreateModule(\"" + m.Class + "\", \"" + modFullName + "\")");
                                    }
                                }
                                //if (!m.ConfPos.IsEmpty) w.Write(" confpos=\"" + m.ConfPos.ToString() + "\"");
                                if (m.Params != null) {
                                    foreach (Param p in m.Params) {
                                        w.WriteLine("mmSetParamValue(\"" + modFullName + "::" + p.Name + "\", \"" + SafeString(p.Value) + "\")");
                                    }
                                }
                            }
                        }
                        if (v.Calls != null) {
                            foreach (Call c in v.Calls) {
                                w.WriteLine("mmCreateCall(\"" + c.Class + "\", \"" + "::" + v.Name + "::" + c.FromModule.Name + "::" + c.FromSlot 
                                            + "\", \"" + "::" + v.Name + "::" + c.ToModule.Name + "::" + c.ToSlot + "\")");
                            }
                        }
                    }
                }
            }
        }

        internal void LoadFromLua(string filename) {
            string[] lines = System.IO.File.ReadAllLines(filename);
            List<View> vs = new List<View>();

            foreach(string line in lines)
            {
                if(line.Contains("mmCreateView"))
                {
                    View v = new View();
                    loadViewFromLua(ref v, lines);
                    vs.Add(v);
                }
            }

            Views = vs.ToArray();
            GeneratorComment = null;
            StartComment = null;
        }

        private void loadViewFromLua(ref View view, string[] lines) {
            // TODO: exception checks needed, currently no checks such as "throw new Exception("Module without class encountered")" are made
            List<Module> modules = new List<Module>();
            List<Call> calls = new List<Call>();

            // first, search and find all modules in the .lua
            // now SetParamValue and CreateCall can be called before CreateModule without causing problems
            for (int i = 0; i < lines.Length; ++i)
            {
                string line = lines[i];

                if (line.Contains("mmCreateView"))
                {
                    Module m = new Module();
                    string[] elements = line.Split('"');
                    string vName = elements[1]; view.Name = vName;
                    string mModuleClass = elements[3]; m.Class = mModuleClass;
                    string[] mModuleFullName = elements[5].Split(':');
                    m.Name = mModuleFullName.Length == 3 ? mModuleFullName[2] : mModuleFullName[0];

                    if (line.Contains("--confPos"))
                    {
                        string[] posLine = line.Split(new string[] { "--" }, StringSplitOptions.None);
                        try
                        {
                            Match pt = Regex.Match(posLine[1], @"\{\s*X\s*=\s*([-.0-9]+)\s*,\s*Y\s*=\s*([-.0-9]+)\s*\}");
                            if (!pt.Success) throw new Exception();
                            m.ConfPos = new Point(
                                int.Parse(pt.Groups[1].Value),
                                int.Parse(pt.Groups[2].Value));
                        }
                        catch { }
                    }

                    modules.Add(m);

                    continue;
                }

                if (line.Contains("mmCreateModule"))
                {
                    Module m = new Module();
                    string[] elements = line.Split('"');
                    string mModuleClass = elements[1]; m.Class = mModuleClass;
                    string[] mModuleFullName = elements[3].Split(':');
                    m.Name = mModuleFullName.Length == 3 ? mModuleFullName[2] : mModuleFullName[4];

                    if (line.Contains("--confPos"))
                    {
                        string[] posLine = line.Split(new string[] { "--" }, StringSplitOptions.None);
                        try
                        {
                            Match pt = Regex.Match(posLine[1], @"\{\s*X\s*=\s*([-.0-9]+)\s*,\s*Y\s*=\s*([-.0-9]+)\s*\}");
                            if (!pt.Success) throw new Exception();
                            m.ConfPos = new Point(
                                int.Parse(pt.Groups[1].Value),
                                int.Parse(pt.Groups[2].Value));
                        }
                        catch { }
                    }

                    modules.Add(m);

                    continue;
                }
            }

            // second, search for all SetParamValue and CreateCall
            // add the found params to the according module and create the correct calls
            for (int i = 0; i < lines.Length; ++i)
            {
                string line = lines[i];

                if (line.Contains("mmSetParamValue"))
                {

                    List<Param> prms = new List<Param>();
                        
                    Param p = new Param();

                    //string[] paramElements = line.Split('"');
                    // can't just split at '"' because of occuring quotes in parameter values
                    // so the string gets split only at quotes which encloses the parameter values
                    // this can be done better with a proper regex pattern that splits only at quotes which has no preceeding backslash
                    // and does not also split the character before the quotes (this is currently done)
                    string[] paramElementsTemp = Regex.Split(line, @"([^\\])\""");
                    string[] paramElements = new string[paramElementsTemp.Length / 2];
                    for(int j = 0; j < paramElementsTemp.Length / 2; ++j)
                    {
                        paramElements[j] = Regex.Replace(paramElementsTemp[2 * j] + paramElementsTemp[2 * j + 1], @"\\+\""", "\"");
                    }
                    string[] paramFullName = paramElements[1].Split(':');

                    // we need to put together the full parameter name that was previously split up
                    // paramName will always start at the 7th position and each consecutive name element 
                    // has an offset of 2, so the paramName is [6]::[8]:: ... ::[6 + 2i]
                    string pName = paramFullName[6];
                    for (int j = 8; j < paramFullName.Length; j += 2)
                    {
                        pName += "::" + paramFullName[j];
                    }
                    p.Name = pName;
                    string pValue = paramElements[3];

                    // replace every sequence of "\\\\...\\\\" with "\\" so the correct filepathes get saved
                    if (pName.Equals("filename"))
                    {
                        pValue = Regex.Replace(pValue, @"\\+", @"\");
                    }
                    
                    p.Value = pValue;

                    // TODO?: instead of [4] do [x] to account for different paramFullNameLengths
                    foreach (Module mTemp in modules)
                    {
                        if (paramFullName[4].StartsWith(mTemp.Name))
                        {
                            List<Param> prmsTemp = new List<Param>();
                            if (mTemp.Params != null)
                            {
                                foreach (Param pTemp in mTemp.Params)
                                {
                                    prmsTemp.Add(pTemp);
                                }
                                prmsTemp.Add(p);
                                mTemp.Params = (prmsTemp.Count == 0) ? null : prmsTemp.ToArray();
                            }
                            else
                            {
                                prmsTemp.Add(p);
                                mTemp.Params = (prmsTemp.Count == 0) ? null : prmsTemp.ToArray();
                            }
                        }
                    }

                    continue;
                }

                if (line.Contains("mmCreateCall"))
                {
                    Call c = new Call();
                    string[] elements = line.Split('"');
                    string cClass = elements[1];
                    string[] cFrom = elements[3].Split(':');
                    string cFromModuleName = cFrom.Length == 5 ? cFrom[2] : cFrom[4];
                    string cFromSlot = cFrom.Length == 5 ? cFrom[4] : cFrom[6];
                    string[] cTo = elements[5].Split(':');
                    string cToModuleName = cTo.Length == 5 ? cTo[2] : cTo[4];
                    string cToSlot = cTo.Length == 5 ? cTo[4] : cTo[6];

                    c.Class = cClass;

                    foreach (Module m in modules)
                    {
                        if (cFromModuleName.StartsWith(m.Name)) {
                            c.FromModule = m;
                            c.FromSlot = cFromSlot;
                        }
                        if (cToModuleName.StartsWith(m.Name)) {
                            c.ToModule = m;
                            c.ToSlot = cToSlot;
                        }
                    }
                    calls.Add(c);

                    continue;
                }
            }

            view.Modules = (modules.Count == 0) ? null : modules.ToArray();
            view.Calls = (calls.Count == 0) ? null : calls.ToArray();
            view.Params = null; // HAZARD: currently not supported


            //    if (!n.HasAttribute("name")) throw new Exception("View without name encountered");

            //    view.Name = n.Attributes["name"].Value;
            //    string viewmodinstname = n.HasAttribute("viewmod") ? n.Attributes["viewmod"].Value : null;

            //    List<Module> modules = new List<Module>();
            //    List<Call> calls = new List<Call>();

            //    foreach (System.Xml.XmlNode c in n.ChildNodes) {
            //        if (c.NodeType != System.Xml.XmlNodeType.Element) continue;
            //        System.Xml.XmlElement oe = (System.Xml.XmlElement)c;
            //        if (oe.Name == "module") {
            //            Module m = new Module();
            //            if (!oe.HasAttribute("class")) throw new Exception("Module without class encountered");
            //            if (!oe.HasAttribute("name")) throw new Exception("Module without name encountered");
            //            m.Class = oe.Attributes["class"].Value;
            //            m.Name = oe.Attributes["name"].Value;
            //            if (oe.HasAttribute("confpos")) {
            //                try {
            //                    Match pt = Regex.Match(oe.Attributes["confpos"].Value, @"\{\s*X\s*=\s*([-.0-9]+)\s*,\s*Y\s*=\s*([-.0-9]+)\s*\}");
            //                    if (!pt.Success) throw new Exception();
            //                    m.ConfPos = new Point(
            //                        int.Parse(pt.Groups[1].Value),
            //                        int.Parse(pt.Groups[2].Value));
            //                } catch { }
            //            }

            //            List<Param> prms = new List<Param>();

            //            foreach (System.Xml.XmlNode oc in oe.ChildNodes) {
            //                if (oc.NodeType != System.Xml.XmlNodeType.Element) continue;
            //                System.Xml.XmlElement oce = (System.Xml.XmlElement)oc;
            //                if (oce.Name != "param") continue;
            //                if (!oce.HasAttribute("name")) throw new Exception("Param without name encountered");

            //                Param p = new Param();
            //                p.Name = oce.Attributes["name"].Value;
            //                p.Value = (oce.HasAttribute("value")) ? oce.Attributes["value"].Value : string.Empty;
            //                prms.Add(p);
            //            }

            //            m.Params = (prms.Count == 0) ? null : prms.ToArray();

            //            modules.Add(m);
            //            if (m.Name == viewmodinstname) {
            //                view.ViewModule = m;
            //            }
            //        }
            //        if (oe.Name == "call") {
            //            Call cl = new Call();
            //            if (!oe.HasAttribute("class")) throw new Exception("Call without class encountered");
            //            cl.Class = oe.Attributes["class"].Value;
            //            if (!oe.HasAttribute("from")) throw new Exception("Call without source encountered");
            //            string callfromname = oe.Attributes["from"].Value;
            //            if (!oe.HasAttribute("to")) throw new Exception("Call without destination encountered");
            //            string calltoname = oe.Attributes["to"].Value;

            //            foreach (Module m in modules) {
            //                if (callfromname.StartsWith(m.Name)) {
            //                    cl.FromModule = m;
            //                    Debug.Assert(callfromname[m.Name.Length] == ':');
            //                    Debug.Assert(callfromname[m.Name.Length + 1] == ':');
            //                    cl.FromSlot = callfromname.Substring(m.Name.Length + 2);
            //                }
            //                if (calltoname.StartsWith(m.Name)) {
            //                    cl.ToModule = m;
            //                    Debug.Assert(calltoname[m.Name.Length] == ':');
            //                    Debug.Assert(calltoname[m.Name.Length + 1] == ':');
            //                    cl.ToSlot = calltoname.Substring(m.Name.Length + 2);
            //                }
            //            }
            //            calls.Add(cl);
            //        }
            //    }

            //    view.Modules = (modules.Count == 0) ? null : modules.ToArray();
            //    view.Calls = (calls.Count == 0) ? null : calls.ToArray();
            //    view.Params = null; // HAZARD: currently not supported
        }

    }

}
