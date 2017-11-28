from spack import *
import platform

class Puk(AutotoolsPackage):
    """Puk Padico micro-kernel"""
    homepage = "http://pm2.gforge.inria.fr/"

    version('trunk', svn='https://scm.gforge.inria.fr/anonscm/svn/padico/PadicoTM/trunk/PadicoTM/Puk')

    resource(
        name='building-tools',
        svn='https://scm.gforge.inria.fr/anonscm/svn/padico/PadicoTM/trunk/PadicoTM/building-tools',
        destination='.'
    )
    
    variant('debug', default=False, description='Build in debug mode')
    variant('optimize', default=True, description='Build in optimized mode')

    depends_on("expat")
    depends_on('pkgconfig', type='build')
    depends_on('autoconf', type='build')

    build_directory = 'build'
    
    def configure_args(self):
        spec = self.spec

        config_args = [
            '--disable-trace',
            ]
        
        config_args.extend([
            "--%s-debug"         % ('enable' if '+debug'     in spec else 'disable'),
            "--%s-optimize"      % ('enable' if '+optimize'  in spec else 'disable'),
        ])

        return config_args
    
    def autoreconf(self, spec, prefix):
        autogen = Executable("./autogen.sh")
        autogen()
            
        
