Imports System.Text, System.IO, System.Xml
Imports System.Globalization

Public Class readConfig

    Private path As String
    '
    '   If the test settings file does not exist, then create a default
    '
    Private Sub create_TestSettings_XML_file(ByVal p As String)
        ' Create XmlWriterSettings.
        Dim settings As XmlWriterSettings = New XmlWriterSettings()
        settings.Indent = True

        ' Create XmlWriter.
        Using writer As XmlWriter = XmlWriter.Create(p, settings)
            ' Begin writing.
            writer.WriteStartDocument()
            writer.WriteStartElement("settings") ' Root.

            ' Loop over employees in array.
            writer.WriteElementString("plcTestThreshold", HomeScreen.plcTestThreshold)
            writer.WriteElementString("rfTestThreshold", HomeScreen.rfTestThreshold)
            writer.WriteElementString("channel", HomeScreen.RF_CHANNEL.ToString("x"))
            writer.WriteElementString("startMacAddr", "FF:FF:FF:FF:FF:FF")

            ' End document.
            writer.WriteEndElement()
            writer.WriteEndDocument()
        End Using
    End Sub

    Public Sub readXML()

        Try
            path = System.IO.Path.Combine(HomeScreen.rootFilePath, "TestSettings.xml")

            '   If settings file is not available
            '   Then create new
            If Not My.Computer.FileSystem.FileExists(path) Then
                MsgBox("Test Settings file not found !", MsgBoxStyle.Critical)
                'create_TestSettings_XML_file(path)
                Return
            End If

            Using xmlString As StreamReader = New StreamReader(path)

                Using reader As XmlReader = XmlReader.Create(New StringReader(xmlString.ReadToEnd.ToString))
                    While reader.Read

                        If reader.Name = "plcTestThreshold" Then
                            If reader.Read Then
                                'MessageBox.Show(reader.Value.ToString)
                                HomeScreen.plcTestThreshold = reader.ReadContentAsInt
                                reader.Read()
                            End If
                        End If

                        If reader.Name = "rfTestThreshold" Then
                            If reader.Read Then
                                'MessageBox.Show(reader.Value.ToString)
                                HomeScreen.rfTestThreshold = reader.ReadContentAsInt
                                reader.Read()
                            End If
                        End If

                        If reader.Name = "channel" Then
                            If reader.Read Then
                                'MessageBox.Show(reader.Value.ToString)
                                reader.Read()
                            End If
                        End If

                        ' Read MAC Address start to 
                        If reader.Name = "startMacAddr" Then
                            If reader.Read Then
                                Dim str As String = String.Join("", reader.Value.ToString.Split(":"))
                                HomeScreen.gMACcounter = ULong.Parse(str, NumberStyles.HexNumber, CultureInfo.CurrentCulture.NumberFormat)
                                HomeScreen.txtbx_MACAddr.Text = reader.Value.ToString
                                'Dim t As String = Hex(HomeScreen.gMACcounter)
                                'Dim i As Integer = 2
                                'Dim s As StringBuilder = New StringBuilder(t)
                                'While True
                                '    s.Insert(i, ":")
                                '    i += 3
                                '    If i >= s.Length Then
                                '        Exit While
                                '    End If
                                'End While
                                reader.Read()
                            End If
                        End If

                    End While
                End Using
            End Using
        Catch ex As Exception
            MessageBox.Show(ex.ToString)
        End Try

    End Sub

    Public Sub create_LogMACAddr_XML_file(ByVal p As String, ByVal macAddr As String)
        ' Create XmlWriterSettings.
        Dim settings As XmlWriterSettings = New XmlWriterSettings()
        settings.Indent = True

        ' Create XmlWriter.
        Using writer As XmlWriter = XmlWriter.Create(p, settings)
            ' Begin writing.
            writer.WriteStartDocument()
            writer.WriteStartElement("MAC_Address") ' Root.

            ' Loop over employees in array.
            writer.WriteElementString("Last_MacAddr", macAddr)

            ' End document.
            writer.WriteEndElement()
            writer.WriteEndDocument()
        End Using
    End Sub
End Class

