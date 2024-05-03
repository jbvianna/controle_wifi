/** @file portaria_remota.js Portaria Remota
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
function limparResultados() {
  linhas_resultado = [];

  txResultados.innerHTML = ''; 
}


/** Mostra o texto na área de resultados/mensagens.
 */
function mostrarResultados() {
  /* Preenche campo de resultados na tela a partir de Array de linhas.
     O campo destino deve ser do tipo <p>.
  */
  txResultados.innerHTML = linhas_resultado.join('<br />\n'); 
}


/** Imprime uma nova mensagem.

    @param texto Nova mensagem. Pode conter várias linhas separadas por '\n'.
 */
function imprimir(texto) {
  
  if (texto.indexOf('\n') < 0) {
    linhas_resultado.push(texto);
  } else {
    for (let linha of texto.split('\n')) {
      linhas_resultado.push(linha);
    }
  }
}


// Persistência de dados no cliente
const chave_registro_base = 'jvianna.gmail.com.portaria.';


function guardarRegistroPermanente(chave, valor = '') {
  localStorage.setItem(chave_registro_base + chave.toString(), JSON.stringify(valor));
}


function lerRegistroPermanente(chave) {
  var repr = localStorage.getItem(chave_registro_base + chave.toString());
  
  if ((typeof repr) =='string') {
    return JSON.parse(repr);
  } else {
    return null;
  }
}


/** Correspondente a JSON.stringify() para text/plain
 */
function objetoParaListaParametros(objeto) {
  let parametros = [];
  
  for (let chave in objeto) {
    parametros.push(chave + '=' + objeto[chave]);
  }
  return parametros;
}


/* Lógica do programa
*/

/** Classe que modela um Sensor do micro-controlador
 */
class Sensor {
  constructor(id) {
    this.servidor = '';                 // Servidor para onde são dirigidas as solicitações
    this.id = id;                       // Identificador do periférico no servidor
    this.valor = 0;                     // Cópia do valor lido no micro-controlador
  }
  
  obterValor() {
    return this.valor;
  }
  
  mudarServidor(serv) {
    this.servidor = serv;
  }

  // Para uso interno no método ler()  
  mudarValor(val) {
    this.valor = val;
  }
  
  /** Envia um comando GET /sensor?id=(id) para o servidor.
  
      O valor lido do sensor é guardado internamente de acordo com a resposta recebida.
   */
  ler() {
    if (this.servidor == '') {
      // Se não sabe quem é o servidor...
      return;
    }
    var xhttp = new XMLHttpRequest();
    
    var comando = this.servidor + 'sensor?id=' + this.id;
    
    var euMesmo = this;
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4) {
        if (this.status == 200) {
          euMesmo.mudarValor(parseInt(this.responseText));
        } else {
          console.log('Erro Sensor::ler() ' + this.status.toString() + ' - ' + this.statusText);
        }
      }
    };
    // console.log('Sensor::ler() enviando comando: ' + comando);
    
    xhttp.open('GET', comando, true);
    xhttp.send();
  }
}


/** Classe que modela um Atuador do micro-controlador
 */
class Atuador {
  constructor(id) {
    this.servidor = '';                 // Servidor para onde são dirigidas as solicitações
    this.id = id;                       // Identificador do periférico no servidor
    this.valor = 0;                     // Guarda o estado esperado do atuador, conforme os comandos enviados
    this.duracao = 0;                   // Valor configurável, usado na ação de pulsar()
  }
  
  mudarServidor(serv) {
    this.servidor = serv;
  }
  
  obterValor() {
    return this.valor;
  }
  
  mudarDuracao(t) {
    this.duracao = t;
  }

  mudarValor(val) {
    this.valor = val;
  }
  
  /** Envia um comando POST /atuador(id) para o servidor.
  
      @param acao ("off" para desligar, "on" para ligar, "toggle" para alternar e
                   "pulse" para ligar e desligar após duração em ms)
   */
  enviarComando(acao) {
    if (this.servidor == '') {
      // Se não sabe quem é o servidor...
      return;
    }
    var xhttp = new XMLHttpRequest();
    
    var cmd = {};
    
    cmd.action = acao;
    
    if (acao == 'pulse') {
      cmd.duracao = this.duracao;
    }
    var conteudo = objetoParaListaParametros(cmd).join('\n');
  
    xhttp.onload = function() {
      if (this.readyState == 4) {
        if (this.status >= 200 && status <= 204) {
          // console.log('Sucesso ' + this.statusText);
        } else {
          console.log('Erro Atuador::enviarComando ' + this.status.toString() + ' - ' + this.statusText);
        }
      }
    };
   
    xhttp.open('POST', this.servidor + 'atuador' + this.id);
    xhttp.setRequestHeader("Content-Type", "text/plain; charset=UTF-8")
    xhttp.send(conteudo);
  }
  
  desligar() {
    this.enviarComando('off');
    this.valor = 0;
  }
  
  ligar() {
    this.enviarComando('on');
    this.valor = 1;
  }
  
  alternar() {
    this.enviarComando('toggle');
    this.valor = 1 - this.valor;
  }
  
  pulsar() {
    this.enviarComando('pulse');
    this.valor = 0;
  }
}


// Para contar por quanto tempo cada atuador permanece ligado (AFAZER - Mudar lógica para o servidor)
var abrindoCancela = 0;
var fechandoCancela = 0;
var soandoSirene = 0;


// Sensores no circuito do ESP_32

// Sensor de id "1" é uma campainha
var campainha = new Sensor("1");

// Sensor de id "2" verifica se uma porta está aberta
var portaAberta = new Sensor("2");


// Atuadores no circuito do ESP_32

// Atuador de id "1" é um botão que deverá aceita comandos "pulse" (AFAZER)
var atAbrirCancela = new Atuador("1");

var atFecharCancela = new Atuador("2");

var sirene = new Atuador("3");

// Atuador de id "4" é um refletor que aceita comandos "toggle"
var refletor = new Atuador("4");



/** Enviar comando para o atuador que controla a cancela para abri-la.
 */
function abrirCancela(evento) {
  atAbrirCancela.ligar();
  abrindoCancela = 2;
}

btAbrir.addEventListener('click', abrirCancela);


/** Enviar comando para o atuador que controla a cancela para fechá-la.
 */
function fecharCancela(evento) {
  atFecharCancela.ligar();
  fechandoCancela = 2;
}

btFechar.addEventListener('click', fecharCancela);


/** Forçar silêncio na sirene
 */
function pararSirene() {
  sirene.desligar();
  btSirene.value = 'Soar Sirene';
  soandoSirene = 0;
}


/** Trata botão que controla a sirene (toggle). 
 */
function tratarSirene(evento) {
  if (soandoSirene > 0) {
    pararSirene();
  } else {
    sirene.ligar();
    btSirene.value = 'Parar Sirene';
    soandoSirene = 5;
  }
}

btSirene.addEventListener('click', tratarSirene);


/** Trata botão que controla o refletor (toggle). 
 */
function tratarRefletor(evento) {
  refletor.alternar();

  if (refletor.obterValor() == 1) {
    btRefletor.style.background = "rgb(240,240,112)";
  } else {
    btRefletor.style.background = "rgb(240,240,240)";
  }
}

btRefletor.addEventListener('click', tratarRefletor);


/* Comunicação com o servidor
*/
var servidorConectado = false;


/* Telas do programa
*/
function abrirTelaConfig(evento) {
  lsModoWifi.selectedIndex = '0';
  txSenha.value = '';
  
  telaPrincipal.style.display = 'none';
  telaConfig.style.display = 'block';
}

btConfigurar.addEventListener('click', abrirTelaConfig);


function cancelarConfig(evento) {
  telaPrincipal.style.display = 'block';
  telaConfig.style.display = 'none';
}

btCancelar.addEventListener('click', cancelarConfig);


function salvarConfig(evento) {
  var serv = txURI.value;
  
  var xhttp = new XMLHttpRequest();
  
  var cfg = {};
  
  cfg.ssid = txSSId.value;
  cfg.password = txSenha.value;
  cfg.hostname = txHostname.value;
  cfg.modo_wifi = lsModoWifi.value;
  
  var conteudo = objetoParaListaParametros(cfg).join('\n');

  xhttp.onload = function() {
    if (this.readyState == 4) {
      if (this.status >= 200 && this.status <= 204) {
        servidorConectado = false;
        limparResultados();
      } else {
        console.log('Erro salvarConfig: ' + this.status.toString() + ' - ' + this.statusText);
      }
    }
  };
  
  xhttp.open('POST', serv + 'config');
  xhttp.setRequestHeader("Content-Type", "text/plain; charset=UTF-8")
  xhttp.send(conteudo);
  
  telaPrincipal.style.display = 'block';
  telaConfig.style.display = 'none';
}

btSalvar.addEventListener('click', salvarConfig);



function refrescarTela() {
  if (servidorConectado) {
    if (abrindoCancela > 0) {
      abrindoCancela -= 1;
      if (abrindoCancela <= 0) {
        atAbrirCancela.desligar();
      }
    }
    if (fechandoCancela > 0) {
      fechandoCancela -= 1;
      if (fechandoCancela <= 0) {
        atFecharCancela.desligar();
      }
    }
    if (soandoSirene > 0) {
      soandoSirene -= 1;
      if (soandoSirene <= 0) {
        pararSirene();
      }
    }
    campainha.ler();
    portaAberta.ler();
    
    if (campainha.obterValor() == 0) {
      // Campainha está acionada
      txCampainha.style = "display: block; color: blue;";
    } else {
      txCampainha.style = "display: none;";
    }
    if (portaAberta.obterValor() != 0) {
      // A porta está aberta
      txPorta.style = "display: block; color: red;";
    } else {
      txPorta.style = "display: none;";
    }
    mostrarResultados();
  } else {
    verificarConexao();
  }
}

// Para refrescar a tela automaticamente
var scrollId;
var refrescandoTela = false;

function reiniciarControlador() {
  atAbrirCancela.desligar();
  abrindoCancela = 0;

  atFecharCancela.desligar();
  fechandoCancela = 0;

  pararSirene();
}

/** Verifica se a conexão está ativa enviando uma solicitação GET /status
 */
function verificarConexao() {
  var serv = txURI.value;
  
  var xhttp = new XMLHttpRequest();

  // Limpa tela antes de enviar comando.
  limparResultados();
  
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4) {
      if (this.status == 200) {
        let resposta = this.responseText; 

        // Se está tudo certo, imprimir o texto recebido.
        limparResultados();
        imprimir(resposta);
        
        servidorConectado = true;
      } else {
        servidorConectado = false;
        console.log('Erro verificarConexao: ' + this.status.toString() + ' - ' + this.statusText);
      }
    }
  };
  xhttp.open('GET', serv + 'status', true);
  xhttp.send();
}


function ativarTela() {
  if (refrescandoTela) {
    clearInterval(scrollId);
    refrescandoTela = false;
  }
  var intervalo = 1000; // Refresca a cada 1 segundo
    
  scrollId = setInterval(function () {
    refrescarTela();
  }, intervalo);
  refrescandoTela = true;
  
  btAbrir.disabled = false;
  btFechar.disabled = false;
  btSirene.disabled = false;
  btRefletor.disabled = false;
}


function suspenderTela() {
  if (refrescandoTela) {
    clearInterval(scrollId);
    refrescandoTela = false;
  }
  btAbrir.disabled = true;
  btFechar.disabled = true;
  btSirene.disabled = true;
  btRefletor.disabled = true;
}


function atualizarServPerifericos() {
  campainha.mudarServidor(txURI.value); 
  portaAberta.mudarServidor(txURI.value);

  atAbrirCancela.mudarServidor(txURI.value);
  atFecharCancela.mudarServidor(txURI.value);  
  sirene.mudarServidor(txURI.value);
  refletor.mudarServidor(txURI.value);
}


function guardarURI() {
  guardarRegistroPermanente("id-uri", txURI.value);
  atualizarServPerifericos();
}


function lembrarURI() {
  var servidor = lerRegistroPermanente("id-uri");
  
  if (servidor !== null) {
    txURI.value = servidor;
  }
  atualizarServPerifericos();
}


function tratarModifURI(evento) {
  if (txURI.readOnly) {
    btMudarURI.value = "Reativar";
    
    servidorConectado = false;
    suspenderTela();

    limparResultados();

    txURI.readOnly = false;
  } else {
    txURI.readOnly = true;

    btMudarURI.value = "Alterar";
    
    guardarURI();

    verificarConexao();

    ativarTela();
  }
}

btMudarURI.addEventListener('click', tratarModifURI);


function prepararTelaPrincipal() {
  lembrarURI();
  
  verificarConexao();

  reiniciarControlador();
  
  ativarTela();
}


// Ponto de partida do código.
window.addEventListener('load', prepararTelaPrincipal);
