/* Aplicativo - Portaria Remota
 * descrição: Permite o controle de um módulo micro-controlador para
 *  acionar botões e monitorar sensores em uma portaria remota.
 * autor: João Vianna (jvianna@gmail.com)
 * data: 2024-04-27
 * versão: 0.7.0
 */
'use strict';

/* Componentes relacionados às telas do programa
*/
// Telas do programa
const telaPrincipal = document.getElementById('id-tela-principal');
const telaConfig = document.getElementById('id-tela-config');

// Botões, caixas de texto, listas...
const btAbrir = document.getElementById('id-abrir');
const btFechar = document.getElementById('id-fechar');
const btSirene = document.getElementById('id-sirene');
const btRefletor = document.getElementById('id-refletor');
const btMudarURI = document.getElementById('id-mudar-uri');

const btConfigurar = document.getElementById('id-configurar');
const btSalvar = document.getElementById('id-salvar-config');
const btCancelar = document.getElementById('id-cancelar');

const txURI = document.getElementById('id-uri');
const lsModoWifi = document.getElementById('ls-modo-wifi');

const txSSId = document.getElementById('id-ssid');
const txSenha = document.getElementById('id-password');
const txHostname = document.getElementById('id-hostname');

// Mensagens
const txCampainha = document.getElementById('tx-campainha');
const txPorta = document.getElementById('tx-porta');
const txResultados = document.getElementById('tx-resultados');


/* Funções auxiliares para apresentação de resultados
 */
var linhas_resultado = [];


/** Limpa o texto na área de resultados/mensagens.
 */
function LimparResultados() {
  linhas_resultado = [];

  txResultados.innerHTML = ''; 
}


/** Mostra o texto na área de resultados/mensagens.
 */
function MostrarResultados() {
  /* Preenche campo de resultados na tela a partir de Array de linhas.
     O campo destino deve ser do tipo <p>.
  */
  txResultados.innerHTML = linhas_resultado.join('<br />\n'); 
}


/** Imprime uma nova mensagem.

    @param texto Nova mensagem. Pode conter várias linhas separadas por '\n'.
 */
function Imprimir(texto) {
  
  if (texto.indexOf('\n') < 0) {
    linhas_resultado.push(texto);
  } else {
    for (let linha of texto.split('\n')) {
      linhas_resultado.push(linha);
    }
  }
}

/* Lógica do programa
*/
var abrindoCancela = 0;
var fechandoCancela = 0;
var soandoSirene = 0;
var refletorLigado = false;

// Atuadores no circuito do ESP_32
var atAbrirCancela = 1; 
var atFecharCancela = 2; 
var atSirene = 3;
var atRefletor = 4;

// Sensores no circuito do ESP_32
var snCampainha = 1;
var snPortaAberta = 2;

/* Comunicação com o servidor
 */


/** Envia um comando GET /atuar... para o servidor.

    @param atuador Número do atuador, de 1 ao máximo de atuadores no módulo de controle.
    @param valor 0 para desligado, 1 para ligado
 */
function EnviarComandoAtuar(atuador, valor) {
  var serv = txURI.value;
  
  var xhttp = new XMLHttpRequest();
  
  var comando = serv + 'atuar?at=' + atuador.toString() + '&v=' + valor.toString();
  
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4) {
      if (this.status == 200) {
        // console.log('Sucesso ' + this.statusText);
      } else {
        console.log('Erro EnviarComando Atuador ' + this.status.toString() + ' - ' + this.statusText);
      }
    }
  };
  // console.log('Enviando comando: ' + comando);
  
  xhttp.open('GET', comando, true);
  xhttp.send();
}

/* Estado dos sensores mantido em memória
 */
var campainhaAtivada = 1;
var portaAberta = 0;            

/** Envia comando GET /ler... para ler o estado da campainha (0 para acionada, 1 caso contrário).
 */
function LerCampainha() {
  var serv = txURI.value;
  
  var xhttp = new XMLHttpRequest();
  
  var comando = serv + 'ler?sn=' + snCampainha.toString();
  
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4) {
      if (this.status == 200) {
        campainhaAtivada = parseInt(this.responseText);
      } else {
        console.log('Erro LerCampainha Atuador ' + this.status.toString() + ' - ' + this.statusText);
      }
    }
  };
  // console.log('Enviando comando: ' + comando);
  
  xhttp.open('GET', comando, true);
  xhttp.send();
}


/** Envia comando GET /ler... para ler o estado da porta (1 para aberta, 0 caso contrário).
 */
function LerPorta() {
  var serv = txURI.value;
  
  var xhttp = new XMLHttpRequest();
  
  var comando = serv + 'ler?sn=' + snPortaAberta.toString();
  
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4) {
      if (this.status == 200) {
        portaAberta = parseInt(this.responseText);
      } else {
        console.log('Erro LerPorta ' + this.status.toString() + ' - ' + this.statusText);
      }
    }
  };
  // console.log('Enviando comando: ' + comando);
  
  xhttp.open('GET', comando, true);
  xhttp.send();
}


/** Enviar comando para o atuador que controla a cancela para abri-la.
 */
function AbrirCancela(evento) {
  EnviarComandoAtuar(atAbrirCancela, 1);
  abrindoCancela = 2;
}

btAbrir.addEventListener('click', AbrirCancela);


/** Enviar comando para o atuador que controla a cancela para fechá-la.
 */
function FecharCancela(evento) {
  EnviarComandoAtuar(atFecharCancela, 1);
  fechandoCancela = 2;
}

btFechar.addEventListener('click', FecharCancela);


/** Forçar silêncio na sirene
 */
function PararSirene() {
  EnviarComandoAtuar(atSirene, 0);
  btSirene.value = 'Soar Sirene';
  soandoSirene = 0;
}


/** Trata botão que controla a sirene (toggle). 
 */
function TratarSirene(evento) {
  if (soandoSirene > 0) {
    PararSirene();
  } else {
    EnviarComandoAtuar(atSirene, 1);
    btSirene.value = 'Parar Sirene';
    soandoSirene = 5;
  }
}

btSirene.addEventListener('click', TratarSirene);


/** Trata botão que controla a sirene (toggle). 
 */
function TratarRefletor(evento) {
  if (refletorLigado) {
    EnviarComandoAtuar(atRefletor, 0);
    btRefletor.style.background = "rgb(240,240,240)";
    refletorLigado = false;
  } else {
    EnviarComandoAtuar(atRefletor, 1);
    btRefletor.style.background = "rgb(240,240,112)";
    refletorLigado = true;
  }
}

btRefletor.addEventListener('click', TratarRefletor);


/* Comunicação com o servidor
*/
var servidorConectado = false;


/* Telas do programa
*/
function AbrirTelaConfig(evento) {
  lsModoWifi.selectedIndex = '0';
  txSenha.value = '';
  
  telaPrincipal.style.display = 'none';
  telaConfig.style.display = 'block';
}

btConfigurar.addEventListener('click', AbrirTelaConfig);


function CancelarConfig(evento) {
  telaPrincipal.style.display = 'block';
  telaConfig.style.display = 'none';
}

btCancelar.addEventListener('click', CancelarConfig);


function SalvarConfig(evento) {
  var serv = txURI.value;
  
  var xhttp = new XMLHttpRequest();
  
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4) {
      if (this.status == 200) {
        servidorConectado = false;
        LimparResultados();
      } else {
        console.log('Erro SalvarConfig: ' + this.status.toString() + ' - ' + this.statusText);
      }
    }
  };
  var modo_ap = '0';

  if (lsModoWifi.value == 'AP') {
    modo_ap = '1';
  }
  
  // Nota: o comando pode ter espaços e outros caracteres que precisam ser codificados.
  var comando = 'config?ssid=' + txSSId.value + '&pw=' + txSenha.value + '&host=' + txHostname.value + '&ap=' + modo_ap;
  
  xhttp.open('GET', encodeURI(serv + comando), true);
  xhttp.send();

  telaPrincipal.style.display = 'block';
  telaConfig.style.display = 'none';
}

btSalvar.addEventListener('click', SalvarConfig);



function RefrescarTela() {
  if (servidorConectado) {
    if (abrindoCancela > 0) {
      abrindoCancela -= 1;
      if (abrindoCancela <= 0) {
        EnviarComandoAtuar(atAbrirCancela, 0);
      }
    }
    if (fechandoCancela > 0) {
      fechandoCancela -= 1;
      if (fechandoCancela <= 0) {
        EnviarComandoAtuar(atFecharCancela, 0);
      }
    }
    if (soandoSirene > 0) {
      soandoSirene -= 1;
      if (soandoSirene <= 0) {
        PararSirene();
      }
    }
    LerCampainha();
    LerPorta();
    
    if (campainhaAtivada == 0) {
      txCampainha.style = "display: block; color: blue;";
    } else {
      txCampainha.style = "display: none;";
    }
    if (portaAberta > 0) {
      txPorta.style = "display: block; color: red;";
    } else {
      txPorta.style = "display: none;";
    }
    MostrarResultados();
  } else {
    VerificarConexao();
  }
}

// Para refrescar a tela automaticamente
var scrollId;
var refrescandoTela = false;

function ReiniciarControlador() {
  EnviarComandoAtuar(atAbrirCancela, 0);
  abrindoCancela = 0;

  EnviarComandoAtuar(atFecharCancela, 0);
  fechandoCancela = 0;

  PararSirene();
}

/** Verifica se a conexão está ativa enviando uma solicitação GET /status
 */
function VerificarConexao() {
  var serv = txURI.value;
  
  var xhttp = new XMLHttpRequest();

  // Limpa tela antes de enviar comando.
  LimparResultados();
  
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4) {
      if (this.status == 200) {
        let resposta = this.responseText; 

        // Se está tudo certo, imprimir o texto recebido.
        LimparResultados();
        Imprimir(resposta);
        
        servidorConectado = true;
      } else {
        servidorConectado = false;
        console.log('Erro VerificarConexao: ' + this.status.toString() + ' - ' + this.statusText);
      }
    }
  };
  xhttp.open('GET', serv + 'status', true);
  xhttp.send();
}


function AtivarTela() {
  if (refrescandoTela) {
    clearInterval(scrollId);
    refrescandoTela = false;
  }
  var intervalo = 1000; // Refresca a cada 1 segundo
    
  scrollId = setInterval(function () {
    RefrescarTela();
  }, intervalo);
  refrescandoTela = true;
  
  btAbrir.disabled = false;
  btFechar.disabled = false;
  btSirene.disabled = false;
  btRefletor.disabled = false;
}


function SuspenderTela() {
  if (refrescandoTela) {
    clearInterval(scrollId);
    refrescandoTela = false;
  }
  btAbrir.disabled = true;
  btFechar.disabled = true;
  btSirene.disabled = true;
  btRefletor.disabled = true;
}


// Persistência de dados no cliente
const chave_registro_base = 'jvianna.gmail.com.portaria.';


function GuardarRegistroPermanente(chave, valor = '') {
  localStorage.setItem(chave_registro_base + chave.toString(), JSON.stringify(valor));
}

function LerRegistroPermanente(chave) {
  var repr = localStorage.getItem(chave_registro_base + chave.toString());
  
  if ((typeof repr) =='string') {
    return JSON.parse(repr);
  } else {
    return null;
  }
}


function GuardarURI() {
  GuardarRegistroPermanente("id-uri", txURI.value);
}


function LembrarURI() {
  var servidor = LerRegistroPermanente("id-uri");
  
  if (servidor !== null) {
    txURI.value = servidor;
  }
}



function TratarModifURI(evento) {
  if (txURI.readOnly) {
    btMudarURI.value = "Reativar";
    
    servidorConectado = false;
    SuspenderTela();

    LimparResultados();

    txURI.readOnly = false;
  } else {
    txURI.readOnly = true;

    btMudarURI.value = "Alterar";
    
    GuardarURI();

    VerificarConexao();

    AtivarTela();
  }
}

btMudarURI.addEventListener('click', TratarModifURI);


function PrepararTelaPrincipal() {
  LembrarURI();
  
  VerificarConexao();

  ReiniciarControlador();
  
  AtivarTela();
}


// Ponto de partida do código.
window.addEventListener('load', PrepararTelaPrincipal);
